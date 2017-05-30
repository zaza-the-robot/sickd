/*
 * sickd - Daemon for collecting data from Sick laser sensor devices
 * This implements packet decoding for PLS201 series sensors.
 *
 * Copyright (C) 2017 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * This file is part of sickd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "sickd.h"

#include <stdio.h>
#include <string.h>

#define SICK_STX				0x02
#define SICK_PKT_HEADER_LEN			4
#define SICK_PKT_CRC_LEN			2
#define SICK_PKT_WRAP_LENGTH			(SICK_PKT_HEADER_LEN \
						+ SICK_PKT_CRC_LEN)
#define MAX_REASONABLE_PACKET_LEN		700

enum sick_packet_type {
	SICK_PKT_TYPE_ID = 0x90,
	SICK_PKT_TYPE_LASER_DISTANCE = 0xb0,
};

const struct sick_driver pls201_driver;

static void pls201_close(struct sick_device *sdev)
{
	if (!sdev)
		return;

	if (sdev->sp_dev) {
		sp_close(sdev->sp_dev);
		sickd_source_remove(sp_fileno(sdev->sp_dev));
	}

	free(sdev);
}

static int pls_create(struct sick_device **sdev, const char *port)
{
	struct sick_device *new_sick;

	*sdev = NULL;

	new_sick = malloc(sizeof(*new_sick));
	if (!new_sick)
		return -ENOMEM;


	if (sp_get_port_by_name(port, &new_sick->sp_dev) != SP_OK) {
		printf("No such serial port %s\n", port);
		return -ENODEV;
	}

	if (sp_open(new_sick->sp_dev, SP_MODE_READ_WRITE) != SP_OK) {
		printf("Could not open serial port\n");
		return sp_last_error_code();
	}

	new_sick->driver = &pls201_driver;
	*sdev = new_sick;
	return 0;
}

static int pls201_open(struct sick_device **sdev, const char *port)
{
	int ret;

	ret = pls_create(sdev, port);
	if (ret < 0)
		pls201_close(*sdev);

	sickd_source_add(*sdev, sp_fileno((*sdev)->sp_dev));

	return ret;
}

static void pls_decode_laser_data(const uint8_t *buf, size_t len)
{
	size_t i, expected_len;
	uint16_t distance[180];
	uint16_t data_len = read_le16(buf + 1);
	const uint8_t *dist_base = buf + 3;

	expected_len = data_len * sizeof(uint16_t) + 4;
	/* TODO: Handle this elegantly */
	if ((len != expected_len) || (data_len != 180))
		return;

	for (i = 0; i < data_len; i++) {
		distance[i] = read_le16(dist_base + i * sizeof (uint16_t));
		distance[i] &= 0x3ff;
	}

	/* TODO: Publish the data somehow. */
	(void) distance;
}

static void pls_decode_payload(const uint8_t *buf, size_t len)
{
	uint8_t type = buf[0];

	switch (type) {
	case SICK_PKT_TYPE_ID:
		printf("ID packet: %.*s\n", len - 1, buf + 1);
		break;
	case SICK_PKT_TYPE_LASER_DISTANCE:
		pls_decode_laser_data(buf, len);
		break;
	default:
		printf("Unknown packet %.02x\n", type);
		return;
	}
}

static size_t pls_process_packet(const uint8_t *buf, size_t len)
{
	uint8_t dest_addr;
	uint16_t payload_len, packet_len, crc, expected_crc;

	if (len < SICK_PKT_HEADER_LEN)
		return 0;

	dest_addr = buf[1];
	payload_len = read_le16(buf + 2);

	/* The packet length field does not include the header and the CRC */
	packet_len = payload_len + SICK_PKT_WRAP_LENGTH;

	if (packet_len > MAX_REASONABLE_PACKET_LEN) {
		printf("Packet too long (%d). Re-syncing.\n", packet_len);
		/* Skip the STX byte, so we re-sync on the next call. */
		return 1;
	}

	/* Don't process the packet if we haven't received all of it. */
	if (len < packet_len)
		return 0;

	crc = crc_sick(buf, len - SICK_PKT_CRC_LEN);
	expected_crc = read_le16(&buf[len - 2]);

	if (crc != expected_crc) {
		printf("CRC mismatch: expected %.04x vs calculated %.04x\n",
		       expected_crc, crc);
		return 1;
	}

	pls_decode_payload(buf + SICK_PKT_HEADER_LEN, payload_len);

	printf("Got a %d bytes packet\n", packet_len);

	return packet_len;
}

/* Returns the number of bytes that have been processed and can be discarded. */
static size_t pls_process_stream(const void *buf, size_t len)
{
	const void *stx_ptr;
	size_t num_skipped;
	uintptr_t buf_start, packet_start;

	stx_ptr = memchr(buf, SICK_STX, len);
	if (!stx_ptr) {
		/* No start byte. Discard everything. */
		return len;
	}

	buf_start = (uintptr_t)buf;
	packet_start = (uintptr_t)stx_ptr;

	num_skipped = packet_start - buf_start;
	num_skipped += pls_process_packet(stx_ptr, len - num_skipped);

	return num_skipped;
}

static int pls201_process_events(struct sick_device *sdev)
{
	size_t processed;
	int num_bytes;
	num_bytes = sp_nonblocking_read(sdev->sp_dev,
					sdev->buf + sdev->buf_data_size,
					sizeof(sdev->buf) - sdev->buf_data_size);
	if (num_bytes < 0) {
		printf("Error reading from serial port\n");
		return -EIO;
	}

	sdev->buf_data_size += num_bytes;

	if (sdev->buf_data_size) {
		processed = pls_process_stream(sdev->buf, sdev->buf_data_size);
		/* Trim the bytes we are told are not needed anymore. */
		memmove(sdev->buf, sdev->buf + processed, sdev->buf_data_size - processed);
		sdev->buf_data_size -= processed;
	}

	if (sdev->buf_data_size == sizeof(sdev->buf)) {
		printf("Buffer full. We are deadlocked. Aborting.\n");
		return -EDEADLOCK;
	}
}

static const struct sick_device_id pls201_ids[] = {
	{ .compatible = "pls201" },
	{ .compatible = "pls201-113" },
	{},
};

const struct sick_driver pls201_driver = {
	.device_ids = pls201_ids,
	.open = pls201_open,
	.close = pls201_close,
	.process_events = pls201_process_events,
};
