#include "sickd.h"

#include <libserialport.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SICK_STX				0x02
#define SICK_PKT_HEADER_LEN			4
#define SICK_PKT_CRC_LEN			2
#define SICK_PKT_WRAP_LENGTH			(SICK_PKT_HEADER_LEN \
						+ SICK_PKT_CRC_LEN)
#define MAX_REASONABLE_PACKET_LEN		700

struct sick_sensor {
	const char *name;
	unsigned int baudrate;
};

static struct sick_sensor sick_pls = {
	.name = "/dev/ttyUSB0",
	.baudrate = 9600,
};

void sick_decode_packet(const uint8_t *buf, size_t len)
{
	/* NOT IMPLEMENTED! */
}

size_t sick_process_packet(const uint8_t *buf, size_t len)
{
	uint8_t dest_addr;
	uint16_t packet_len, crc, expected_crc;

	if (len < SICK_PKT_HEADER_LEN)
		return 0;

	dest_addr = buf[1];
	packet_len = buf[2] | buf[3] << 8;

	/* The packet length field does not include the header and the CRC */
	packet_len += SICK_PKT_WRAP_LENGTH;

	if (packet_len > MAX_REASONABLE_PACKET_LEN) {
		printf("Packet too long (%d). Re-syncing.\n", packet_len);
		/* Skip the STX byte, so we re-sync on the next call. */
		return 1;
	}

	/* Don't process the packet if we haven't received all of it. */
	if (len < packet_len)
		return 0;

	crc = crc_sick(buf, len - SICK_PKT_CRC_LEN);
	expected_crc = buf[len - 1] | buf[len - 2] << 8;

	if (crc != expected_crc)
		return 1;

	sick_decode_packet(buf, packet_len);

	return packet_len;
}

/* Returns the number of bytes that have been processed and can be discarded. */
size_t sick_process_stream(const void *buf, size_t len)
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
	num_skipped += sick_process_packet(stx_ptr, len - num_skipped);

	return num_skipped;
}

int main(int argc, char **argv)
{
	uint8_t buf[1024];
	size_t data_len = 0, processed;
	int num_bytes, ret;
	struct sp_port *sickp;

	if (sp_get_port_by_name(sick_pls.name, &sickp) != SP_OK) {
		printf("No such rerial port\n");
		return EXIT_FAILURE;
	}

	if (sp_open(sickp, SP_MODE_READ_WRITE) != SP_OK) {
		printf("Could not open serial port\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "sickd starting on %s\n", sick_pls.name);

	ret = EXIT_SUCCESS;
	while (1) {
		num_bytes = sp_nonblocking_read(sickp, buf + data_len,
						sizeof(buf) - data_len);
		if (num_bytes < 0) {
			printf("Error reading from serial port\n");
			ret = EXIT_FAILURE;
			break;
		}
		data_len += num_bytes;

		if (data_len) {
			processed = sick_process_stream(buf, data_len);
			/* Trim the bytes we are told are not needed anymore. */
			memmove(buf, buf + processed, data_len - processed);
			data_len -= processed;
		}

		if (data_len == sizeof(buf)) {
			printf("Buffer full. We are deadlocked. Aborting.\n");
			ret = EXIT_FAILURE;
		}

	}

	sp_close(sickp);
	return ret;
}
