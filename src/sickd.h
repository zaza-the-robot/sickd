/*
 * sickd - Daemon for collecting data from Sick laser sensor devices
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

#ifndef __SICKD_H
#define __SICKD_H

#include <errno.h>
#include <libserialport.h>
#include <stdint.h>
#include <stdlib.h>

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof(x[0]))

struct sick_driver;

struct sick_device {
	struct sp_port *sp_dev;
	const struct sick_driver *driver;
	uint8_t buf[1024];
	size_t buf_data_size;
};

struct sick_device_id {
	const char *compatible;
};

struct sick_driver {
	const struct sick_device_id *device_ids;
	int (*open)(struct sick_device **sdev, const char *port);
	void (*close)(struct sick_device *sdev);
	int (*process_events)(struct sick_device *sdev);
};

static inline uint16_t read_le16(const void *buf)
{
	const uint8_t *raw = buf;
	return raw[0] | (raw[1] << 8);
}

static inline int sp_fileno(struct sp_port *sp_dev)
{
	int fd;

	sp_get_port_handle(sp_dev, &fd);
	return fd;
}

typedef int (*pollfd_callback)(struct sick_device *sdev);
void sickd_source_add(struct sick_device *sdev, int pollfd);
void sickd_source_remove(int pollfd);
void sickd_run_events(void);

uint16_t crc_sick(const uint8_t *input_str, size_t num_bytes);

#endif /* __SICKD_H */
