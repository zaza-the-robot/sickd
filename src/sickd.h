/*
 * Copyright (C) 2017 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * This file is part of sickd.
 *
 * sickd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef __SICKD_H
#define __SICKD_H

#include <stdint.h>
#include <stdlib.h>

uint16_t crc_sick(const uint8_t *input_str, size_t num_bytes);

#endif /* __SICKD_H */
