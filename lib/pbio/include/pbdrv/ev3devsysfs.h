// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PBIO_EVDEVSYSFS_H_
#define _PBIO_EVDEVSYSFS_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

pbio_error_t sysfs_get_number(pbio_port_t port, const char *rdir, int *sysfs_number);

pbio_error_t sysfs_open(FILE **file, const char *pathpat, int n, const char *attribute, const char *rw);

pbio_error_t sysfs_open_sensor_attr(FILE **file, int n, const char *attribute, const char *rw);

pbio_error_t sysfs_read_str(FILE *file, char *dest);

pbio_error_t sysfs_write_str(FILE *file, const char *str);

pbio_error_t sysfs_read_int(FILE *file, int *dest);


#endif // _PBIO_EVDEVSYSFS_H_
