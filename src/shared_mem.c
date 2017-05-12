/*
 * sickd - Daemon for collecting data from Sick laser sensor devices
 * Implements the shared memory interface for exporting sensor data.
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

#include <stdatomic.h>
#include <sys/ipc.h>
#include <sys/shm.h>

static int smem_id = -1;
static struct sickd_shmem_area *shm = NULL;

int sickd_smem_init(void)
{
	smem_id = shmget(SICKD_SHMEM_ID, sizeof(*shm), IPC_CREAT | 0 | 0644);
	if (smem_id < 0)
		return -1;

	shm = shmat(smem_id, NULL, 0);
	if (!shm)
		return -1;

	atomic_store(&shm->prepublish_stamp, SICKD_SHMEM_STAMP_NO_DATA);
	atomic_store(&shm->postpublish_stamp, SICKD_SHMEM_STAMP_NO_DATA);
	return 0;
}

int sickd_smem_publish(struct sick_packet *packet)
{
	size_t shm_stamp;

	if (!shm)
		return -1;

	shm_stamp = atomic_load(&shm->prepublish_stamp);
	shm_stamp++;

	/* Handle rollover */
	if (shm_stamp == SICKD_SHMEM_STAMP_NO_DATA)
		shm_stamp = 1;

	atomic_store(&shm->prepublish_stamp, shm_stamp);
	shm->pkt = *packet;
	atomic_store(&shm->postpublish_stamp, shm_stamp);
}

void sickd_shm_release(void)
{
	shmctl(smem_id, IPC_RMID, NULL);
}
