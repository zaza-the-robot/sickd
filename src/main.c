#include "sickd.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

struct sick_sensor {
	const char *compatible;
	const char *port;
	unsigned int baudrate;
};

static struct sick_sensor sick_pls = {
	.compatible = "pls201-113",
	.port = "/dev/ttyUSB0",
	.baudrate = 9600,
};

extern const struct sick_driver pls201_driver;

struct sick_device *open_device(const char *id, const char *port)
{
	const struct sick_device_id *ids;
	struct sick_device *sdev;
	const struct sick_driver *my_drv = &pls201_driver;

	ids = my_drv->device_ids;
	for (ids = my_drv->device_ids; ids->compatible; ids++) {
		if (!strcmp(id, ids->compatible))
			break;
	}

	if (!ids->compatible) {
		fprintf(stderr, "No driver for %s\n", sick_pls.compatible);
		return NULL;
	}

	if (my_drv->open(&sdev, sick_pls.port) < 0) {
		printf("Failed to open sick device!\n");
		return NULL;
	}

	return sdev;
}

int main(int argc, char **argv)
{
	int ret;
	struct sick_device *my_dev;

	fprintf(stderr, "sickd starting on %s\n", sick_pls.port);

	my_dev = open_device(sick_pls.compatible, sick_pls.port);
	if (!my_dev)
		return EXIT_FAILURE;


	ret = EXIT_SUCCESS;
	while (1) {
		if (my_dev->driver->process_events(my_dev) < 0) {
			ret = EXIT_FAILURE;
			break;
		}
	}

	return ret;
}
