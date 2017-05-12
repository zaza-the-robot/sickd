/*
 * libsickd - library for interfacing with the sickd daemon
 * This implements polling of the shared memory interface of sickd.
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

#include "sickd.h"
#include <sys/shm.h>
#include <stdio.h>

static struct{
	size_t last_packet_stamp;
	struct sickd_shmem_area *shmoo;
} shm_ctx = {
	.shmoo = NULL,
};

int libsickd_shmem_init(void)
{
	int shm_id;

	shm_id = shmget(SICKD_SHMEM_ID, sizeof(struct sickd_shmem_area), 0);
	shm_ctx.shmoo = shmat(shm_id, NULL, 0);
	if (!shm_ctx.shmoo) {
		libsickd_shmem_release();
		return -ENOMEM;
	}

	shm_ctx.last_packet_stamp = SICKD_SHMEM_STAMP_NO_DATA;
	return 0;
}

void libsickd_shmem_release(void)
{
	if(shm_ctx.shmoo)
		shmdt(shm_ctx.shmoo);
}

size_t libsickd_shmem_poll(struct sick_packet *pkt)
{
	size_t seq_id;

	seq_id = atomic_load(&shm_ctx.shmoo->prepublish_stamp);
	if (seq_id == shm_ctx.last_packet_stamp)
		return shm_ctx.last_packet_stamp;

	do {
		seq_id = atomic_load(&shm_ctx.shmoo->postpublish_stamp);
		*pkt = shm_ctx.shmoo->pkt;
	} while (seq_id != atomic_load(&shm_ctx.shmoo->prepublish_stamp));

	return seq_id;
}
