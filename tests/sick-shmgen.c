/*
 * sick-shmgen - Generate shared memory data to emulate the sickd daemon
 *
 * Generates fake data packets and exports them over shared memory. It uses the
 * same shared emmory code as sickd, so this can be used to test the shared
 * memory export, libsickd shared memory import, and clients.
 * The plot of the generated data should look like a circle of 500 cm, with
 * some noise, and a 200 cm dip rotating counterclockwise.
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

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static int noise(unsigned int amplitude)
{
	int noise = rand();

	noise -= (RAND_MAX >> 1);
	noise %= amplitude;
}

static void add_dip(struct sick_packet *pkt, size_t seq_id)
{
	const size_t dip_width = 30;
	const size_t depth = 200;

	size_t i;
	size_t dip_start = (seq_id * 10) % ARRAY_SIZE(pkt->distance);

	for (i = dip_start; i < (dip_start + dip_width); i++)
		pkt->distance[i % ARRAY_SIZE(pkt->distance)] -= depth;
}

static void generate_packet(struct sick_packet *pkt, size_t seq_id)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(pkt->distance); i++)
		pkt->distance[i] = 500 + noise(35);

	add_dip(pkt, seq_id);
}

int main(int argc, char **argv)
{
	size_t seq_id = 0;
	struct sick_packet fake_pkt;

	if (sickd_smem_init() < 0)
		return EXIT_FAILURE;

	srand(time(NULL));
	while (1) {
		generate_packet(&fake_pkt, seq_id++);
		sickd_smem_publish(&fake_pkt);
		usleep(500000 + noise(100000));
	}
}
