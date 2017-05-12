/*
 * libsickd - library for interfacing with the sickd daemon
 *
 * Copyright (C) 2017 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * This file is part of libsickd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef LIBSICKD_H
#define LIBSICKD_H

#include <stddef.h>
#include <stdint.h>

#define SICKD_SHMEM_STAMP_NO_DATA	(~0)
/* The string "SICK", when the key is represented in little-endian format */
#define SICKD_SHMEM_ID			0x4b434953

struct sick_packet {
	uint16_t distance[180];
	uint8_t unit;
	uint8_t status;
};

#ifdef __cplusplus
extern "C" {
#endif

int libsickd_shmem_init(void);
void libsickd_shmem_release(void);
size_t libsickd_shmem_poll(struct sick_packet *pkt);

#ifdef __cplusplus
}
#endif

#endif /* LIBSICKD_H */
