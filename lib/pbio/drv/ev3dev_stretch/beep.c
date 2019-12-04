// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>

#include <linux/input.h>
#include <linux/pci.h>

#include <pbio/error.h>

#include <pbdrv/beep.h>

struct _pbdrv_beep_dev_t {
    int fd;
};

static pbdrv_beep_dev_t __beep_dev;

pbio_error_t pbdrv_beep_get(pbdrv_beep_dev_t **_beep_dev) {

    pbdrv_beep_dev_t *beep_dev = &__beep_dev;

    beep_dev->fd = open("/dev/input/by-path/platform-sound-event", O_RDWR, 0);

    if (beep_dev->fd == -1) {
        return PBIO_ERROR_IO;
    }

    *_beep_dev = beep_dev;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_beep_start_freq(pbdrv_beep_dev_t *beep_dev, uint32_t freq) {

    struct input_event event;

    event.type = EV_SND;
    event.code = SND_TONE;
    event.value = freq;

    if (write(beep_dev->fd, &event, sizeof(struct input_event)) == -1) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}
