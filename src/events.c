/*
 * sickd - Daemon for collecting data from Sick laser sensor devices
 * This implements event loop of sickd.
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
#include <poll.h>


#define REALLOC_STEP	10

/* This is a very simple and lightweight file descriptor to active sick device
 * map. Albeit operations being O(n), this is only modified at configure time.
 * 'pfds' is a vector of of pollfd structures designed to be passed as-is to
 * poll(). Each element of 'pfds' has an associated sick device in 'sdev'.
 * 'num' and 'size' describe the number of active events and the allocated
 * capacity, respectively.
 */
static struct pfmap {
	struct pollfd *pfds;
	struct sick_device **sdevs;
	size_t num;
	size_t size;
} pfmap = {
	.pfds = NULL,
	.sdevs = NULL,
	.num = 0,
	.size = 0,

};

static void pfmap_extend(struct pfmap *map)
{
	map->size += REALLOC_STEP;
	map->pfds = realloc(map->pfds, sizeof(*map->pfds) * map->size);
	map->sdevs = realloc(map->sdevs, sizeof(*map->sdevs) * map->size);
}

/* Add a data source to sickd. The file descriptor specified by 'pollfd' is
 * added to the list of events that are polled. the device's "process_events()"
 * function is called when data is available on the specified file descriptor.
 * Note that 'sdev' must not go out of scope while the event source is active.
 */
void sickd_source_add(struct sick_device *sdev, int pollfd)
{
	if (pfmap.num >= pfmap.size)
		pfmap_extend(&pfmap);

	pfmap.sdevs[pfmap.num] = sdev;
	pfmap.pfds[pfmap.num].fd = pollfd;
	pfmap.pfds[pfmap.num].events = POLLIN;
	pfmap.num++;
}

/* Remove data source from sickd. This should be a file descriptor 'pollfd'
 * which was previously added as an event source by "sickd_source_add()".
 * After this call the 'sdev' associated with 'pollfd' may go out of scope.
 */
void sickd_source_remove(int pollfd)
{
	size_t i;

	for (i = 0; i < pfmap.num; i++) {
		if (pfmap.pfds[i].fd != pollfd)
			continue;

		/* Copy the last slot to the current slot. */
		pfmap.num--;
		pfmap.pfds[i] = pfmap.pfds[pfmap.num];
		pfmap.sdevs[i] = pfmap.sdevs[pfmap.num];
	}
}


/* Run the sickd event loop.
 * This is a blocking call, and sleeps when there is no data to process.
 */
void sickd_run_events(void)
{
	size_t i = 0;
	struct sick_device *sdev;

	poll(pfmap.pfds, pfmap.num, 100);

	for (i = 0; i < pfmap.num; i++) {
		if (pfmap.pfds[i].revents == 0)
			continue;

		sdev = pfmap.sdevs[i];
		sdev->driver->process_events(sdev);
	}
}
