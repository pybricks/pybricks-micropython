// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EXPERIMENTAL

#include "py/mphal.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include <pbio/util.h>

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

#include <pybricks/util_pb/pb_error.h>

#include <pybricks/experimental.h>
#include <pybricks/robotics.h>

#if PYBRICKS_HUB_EV3BRICK
#if !MICROPY_MODULE_BUILTIN_INIT
#error "pybricks.experimental module requires that MICROPY_MODULE_BUILTIN_INIT is enabled"
#endif

#include <signal.h>

#include "py/mpthread.h"

STATIC void sighandler(int signum) {
    // we just want the signal to interrupt system calls
}

STATIC mp_obj_t mod_experimental___init__(void) {
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = sighandler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_experimental___init___obj, mod_experimental___init__);

STATIC mp_obj_t mod_experimental_pthread_raise(mp_obj_t thread_id_in, mp_obj_t ex_in) {
    mp_uint_t thread_id = mp_obj_int_get_truncated(thread_id_in);
    if (ex_in != mp_const_none && !mp_obj_is_exception_instance(ex_in)) {
        mp_raise_TypeError(MP_ERROR_TEXT("must be an exception or None"));
    }
    return mp_obj_new_int(mp_thread_schedule_exception(thread_id, ex_in));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_experimental_pthread_raise_obj, mod_experimental_pthread_raise);
#endif // PYBRICKS_HUB_EV3BRICK

#if PYBRICKS_HUB_PRIMEHUB

#include <stm32f4xx_ll_spi.h>
#include <stm32f4xx_hal.h>
#include "lfs.h"

STATIC void flash_enable(bool enable) {
    if (enable) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    }
}

SPI_HandleTypeDef hspi2;

STATIC mp_obj_t experimental_flash_init_spi(void) {

    // SPI2 parameter configuration
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;

    if (HAL_SPI_Init(&hspi2) != HAL_OK) {
        pb_assert(PBIO_ERROR_IO);
    }

    // Enable flash
    flash_enable(true);

    // Command parameters
    uint32_t timeout = 100;
    uint8_t CMD_GET_ID = 0x9F;
    uint8_t id_data[3];
    HAL_StatusTypeDef err;

    // Send ID command
    err = HAL_SPI_Transmit(&hspi2, &CMD_GET_ID, sizeof(CMD_GET_ID), timeout);
    if (err != 0) {
        pb_assert(PBIO_ERROR_IO);
    }

    // Get ID command reply
    err = HAL_SPI_Receive(&hspi2, id_data, sizeof(id_data), timeout);
    if (err != 0) {
        pb_assert(PBIO_ERROR_IO);
    }
    flash_enable(false);

    // Verify flash device ID
    if (id_data[0] != 239 || id_data[1] != 64 || id_data[2] != 25) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(experimental_flash_init_spi_obj, experimental_flash_init_spi);

STATIC HAL_StatusTypeDef flash_status_read(uint8_t *status) {

    uint32_t timeout = 100;

    // Read status command
    uint8_t command = 0x05;

    // Write the read status command
    flash_enable(true);
    HAL_StatusTypeDef err = HAL_SPI_Transmit(&hspi2, &command, sizeof(command), 100);
    if (err != HAL_OK) {
        return err;
    }

    // Receive data
    err = HAL_SPI_Receive(&hspi2, status, 1, timeout);
    flash_enable(false);
    return err;
}

#define FLASH_BUSY (0x01)
#define FLASH_WRITE_ENABLED (0x02)

STATIC HAL_StatusTypeDef flash_wait_ready() {
    uint8_t status = FLASH_BUSY;
    HAL_StatusTypeDef err;
    uint32_t start_time = mp_hal_ticks_ms();

    while (status & (FLASH_BUSY | FLASH_WRITE_ENABLED)) {
        err = flash_status_read(&status);
        if (err != HAL_OK) {
            return err;
        }
        if (mp_hal_ticks_ms() - start_time > 100) {
            return HAL_TIMEOUT;
        }
        MICROPY_EVENT_POLL_HOOK;
    }
    return HAL_OK;
}


STATIC HAL_StatusTypeDef flash_read(uint32_t address, uint8_t *buffer, uint32_t size) {

    uint32_t timeout = 100;

    // First 1MiB is reserved for firmware updates with official firmware.
    address += 1024 * 1024;

    uint8_t address_bytes[5];

    // Read command
    address_bytes[0] = 0x13;

    // Address as big endian
    address_bytes[1] = (address & 0xff000000) >> 24;
    address_bytes[2] = (address & 0x00ff0000) >> 16;
    address_bytes[3] = (address & 0x0000ff00) >> 8;
    address_bytes[4] = address & 0x000000ff;

    // Write the read command with address
    flash_enable(true);
    HAL_StatusTypeDef err = HAL_SPI_Transmit(&hspi2, address_bytes, sizeof(address_bytes), 100);
    if (err != HAL_OK) {
        return err;
    }

    // Receive data
    err = HAL_SPI_Receive(&hspi2, buffer, size, timeout);
    flash_enable(false);
    return err;
}

STATIC HAL_StatusTypeDef write_enable() {
    uint8_t command = 0x06;
    flash_enable(true);
    HAL_StatusTypeDef err = HAL_SPI_Transmit(&hspi2, &command, sizeof(command), 100);
    flash_enable(false);
    return err;
}

STATIC HAL_StatusTypeDef flash_write(uint32_t address, const uint8_t *buffer, uint32_t size) {

    uint32_t timeout = 100;

    // First 1MiB is reserved for firmware updates with official firmware.
    address += 1024 * 1024;

    uint8_t address_bytes[5];

    // Write command
    address_bytes[0] = 0x12;

    // Address as big endian
    address_bytes[1] = (address & 0xff000000) >> 24;
    address_bytes[2] = (address & 0x00ff0000) >> 16;
    address_bytes[3] = (address & 0x0000ff00) >> 8;
    address_bytes[4] = address & 0x000000ff;

    // Enable write mode
    HAL_StatusTypeDef err = write_enable();
    if (err != HAL_OK) {
        return err;
    }

    // Write the write command with address
    flash_enable(true);
    err = HAL_SPI_Transmit(&hspi2, address_bytes, sizeof(address_bytes), timeout);
    if (err != HAL_OK) {
        flash_enable(false);
        return err;
    }

    // Write the data
    err = HAL_SPI_Transmit(&hspi2, (uint8_t *)buffer, size, timeout);
    flash_enable(false);
    return flash_wait_ready();
}

STATIC HAL_StatusTypeDef flash_block_erase(uint32_t address) {

    address += 1024 * 1024;

    // Enable write mode
    HAL_StatusTypeDef err = write_enable();
    if (err != HAL_OK) {
        return err;
    }

    uint8_t address_bytes[5];
    address_bytes[0] = 0x21;
    address_bytes[1] = (address & 0xff000000) >> 24;
    address_bytes[2] = (address & 0x00ff0000) >> 16;
    address_bytes[3] = (address & 0x0000ff00) >> 8;
    address_bytes[4] = address & 0x000000ff;

    // Send erase command
    flash_enable(true);
    err = HAL_SPI_Transmit(&hspi2, address_bytes, sizeof(address_bytes), 100);
    flash_enable(false);

    err = flash_wait_ready();

    return err;
}

int block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {

    lfs_size_t done = 0;

    while (done < size) {

        // How many bytes to read in one go.
        lfs_size_t read_now = size - done;
        if (read_now > c->read_size) {
            read_now = c->read_size;
        }

        // Read chunk of flash.
        if (flash_read(block * c->block_size + off + done, buffer + done, read_now) != HAL_OK) {
            return LFS_ERR_IO;
        }
        done += read_now;

        // Give MicroPython and PBIO some time.
        MICROPY_EVENT_POLL_HOOK;
    }
    return 0;
}

int block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    lfs_size_t done = 0;

    while (done < size) {

        // How many bytes to write in one go.
        lfs_size_t write_now = size - done;

        // Must not be larger than write size
        if (write_now > c->prog_size) {
            write_now = c->prog_size;
        }

        // Must not wrap around the page
        lfs_size_t write_max = 256 - ((off + done) & 0xFF);
        if (write_now > write_max) {
            write_now = write_max;
        }

        // Write chunk of flash.
        if (flash_write(block * c->block_size + off + done, buffer + done, write_now) != HAL_OK) {
            return LFS_ERR_IO;
        }
        done += write_now;

        // Give MicroPython and PBIO some time.
        MICROPY_EVENT_POLL_HOOK;
    }

    return 0;
}

int block_device_erase(const struct lfs_config *c, lfs_block_t block) {
    // Erase block of flash. Block size assumed to match erase size.

    if (flash_block_erase(block * c->block_size) != HAL_OK) {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

int block_device_sync(const struct lfs_config *c) {
    return 0;
}

lfs_t lfs;
lfs_file_t file;

uint8_t lfs_read_buf[512];
uint8_t lfs_prog_buf[512];
uint8_t lfs_lookahead_buf[64];
uint8_t lfs_file_buf[512];

struct lfs_config cfg = {
    .read = block_device_read,
    .prog = block_device_prog,
    .erase = block_device_erase,
    .sync = block_device_sync,

    .read_size = 32,
    .prog_size = 32,
    .block_size = 4096,
    .block_count = 7936,
    .lookahead = 32,

    .read_buffer = lfs_read_buf,
    .prog_buffer = lfs_prog_buf,
    .lookahead_buffer = lfs_lookahead_buf,
    .file_buffer = lfs_file_buf,
};


STATIC mp_obj_t experimental_flash_init_littlefs(void) {

    uint8_t lfs_data[58];

    // Read littlefs data
    if (flash_read(0, lfs_data, sizeof(lfs_data)) != HAL_OK) {
        pb_assert(PBIO_ERROR_IO);
    }

    // Verify magic string for littlefs V1. Anything else is not supported.
    const char *magic = "littlefs";
    if (memcmp(&lfs_data[40], magic, sizeof(magic) - 1) != 0) {
        pb_assert(PBIO_ERROR_NOT_SUPPORTED);
    }

    // Verify block parameters
    uint32_t block_size = pbio_get_uint32_le(&lfs_data[28]);
    uint32_t block_count = pbio_get_uint32_le(&lfs_data[32]);

    if (cfg.block_size != block_size || cfg.block_count != block_count) {
        pb_assert(PBIO_ERROR_NOT_SUPPORTED);
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(experimental_flash_init_littlefs_obj, experimental_flash_init_littlefs);

STATIC mp_obj_t experimental_flash_read_raw(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(address),
        PB_ARG_REQUIRED(len));

    uint32_t read_address = mp_obj_get_int(address_in);
    uint32_t read_len = mp_obj_get_int(len_in);

    // Allocate read data
    uint8_t *read_data = m_malloc(read_len);

    // Read flash
    if (flash_read(read_address, read_data, read_len) != HAL_OK) {
        pb_assert(PBIO_ERROR_IO);
    }

    // Return bytes read
    return mp_obj_new_bytes(read_data, read_len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(experimental_flash_read_raw_obj, 0, experimental_flash_read_raw);


STATIC mp_obj_t experimental_flash_read_file(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_NONE(path));

    // Get file path
    GET_STR_DATA_LEN(path_in, path, path_len);

    // Mount file system
    int lfs_err = lfs_mount(&lfs, &cfg);
    if (lfs_err) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    // Open file
    lfs_err = lfs_file_open(&lfs, &file, (const char *)path, LFS_O_RDONLY);
    switch (lfs_err)
    {
        case LFS_ERR_OK:
            break;
        case LFS_ERR_NOENT:
            mp_raise_OSError(ENOENT);
            break;
        case LFS_ERR_ISDIR:
            mp_raise_OSError(EISDIR);
        default:
            pb_assert(PBIO_ERROR_IO);
            break;
    }

    // Read file contents
    uint8_t *file_buf = m_new(uint8_t, file.size);
    lfs_size_t ret = lfs_file_read(&lfs, &file, file_buf, file.size);
    if (((int)ret) == LFS_ERR_IO) {
        pb_assert(PBIO_ERROR_IO);
    }

    // Clean up
    lfs_file_close(&lfs, &file);
    lfs_unmount(&lfs);

    // Return data
    return mp_obj_new_bytes(file_buf, file.size);
}
// See also experimental_globals_table below. This function object is added there to make it importable.
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(experimental_flash_read_file_obj, 0, experimental_flash_read_file);


STATIC mp_obj_t experimental_flash_write_file(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(path),
        PB_ARG_REQUIRED(data));

    // Get file path
    GET_STR_DATA_LEN(path_in, path, path_len);
    GET_STR_DATA_LEN(data_in, data, data_len);

    // Mount file system
    int lfs_err = lfs_mount(&lfs, &cfg);
    if (lfs_err) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    // Open file
    lfs_err = lfs_file_open(&lfs, &file, (const char *)path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);

    switch (lfs_err)
    {
        case LFS_ERR_OK:
            break;
        case LFS_ERR_ISDIR:
            mp_raise_OSError(EISDIR);
        default:
            pb_assert(PBIO_ERROR_IO);
            break;
    }

    // write file contents
    lfs_size_t ret = lfs_file_write(&lfs, &file, data, data_len);
    if (((int)ret) == LFS_ERR_IO) {
        pb_assert(PBIO_ERROR_IO);
    }

    // Clean up
    lfs_file_close(&lfs, &file);
    lfs_unmount(&lfs);

    return mp_const_none;
}
// See also experimental_globals_table below. This function object is added there to make it importable.
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(experimental_flash_write_file_obj, 0, experimental_flash_write_file);

#endif // PYBRICKS_HUB_PRIMEHUB

// pybricks.experimental.hello_world
STATIC mp_obj_t experimental_hello_world(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        // Add up to 8 arguments below, all separated by one comma.
        // You can choose from:
        //  PB_ARG_REQUIRED(name), This is a required argument.
        //  PB_ARG_DEFAULT_INT(name, value), Keyword argument with default int value.
        //  PB_ARG_DEFAULT_OBJ(name, value), Keyword argument with default MicroPython object.
        //  PB_ARG_DEFAULT_QSTR(name, value), Keyword argument with default string value, without quotes.
        //  PB_ARG_DEFAULT_FALSE(name), Keyword argument with default False value.
        //  PB_ARG_DEFAULT_TRUE(name), Keyword argument with default True value.
        //  PB_ARG_DEFAULT_NONE(name), Keyword argument with default None value.
        PB_ARG_REQUIRED(foo),
        PB_ARG_DEFAULT_NONE(bar));

    // This function can be used in the following ways:
    // from pybricks.experimental import hello_world
    // y = hello_world(5)
    // y = hello_world(foo=5)
    // y = hello_world(5, 6)
    // y = hello_world(foo=5, bar=6)

    // All input arguments are available as MicroPython objects with _in post-fixed to the name.
    // Use MicroPython functions or Pybricks helper functions to unpack them into C types:
    mp_int_t foo = pb_obj_get_int(foo_in);

    // You can check if an argument is none.
    if (bar_in == mp_const_none) {
        // Example of how to print.
        mp_printf(&mp_plat_print, "Bar was not given. Foo is: %d\n", foo);
    }

    // Example of returning an object. Here we return the square of the input argument.
    return mp_obj_new_int(foo * foo);
}
// See also experimental_globals_table below. This function object is added there to make it importable.
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(experimental_hello_world_obj, 0, experimental_hello_world);


STATIC const mp_rom_map_elem_t experimental_globals_table[] = {
    #if PYBRICKS_HUB_EV3BRICK
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_experimental_c) },
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&mod_experimental___init___obj) },
    { MP_ROM_QSTR(MP_QSTR_pthread_raise), MP_ROM_PTR(&mod_experimental_pthread_raise_obj) },
    #else
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_experimental) },
    #endif // PYBRICKS_HUB_EV3BRICK
    #if PYBRICKS_HUB_PRIMEHUB
    { MP_ROM_QSTR(MP_QSTR_flash_init_spi), MP_ROM_PTR(&experimental_flash_init_spi_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_init_littlefs), MP_ROM_PTR(&experimental_flash_init_littlefs_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_read_raw), MP_ROM_PTR(&experimental_flash_read_raw_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_read_file), MP_ROM_PTR(&experimental_flash_read_file_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_write_file), MP_ROM_PTR(&experimental_flash_write_file_obj) },
    #endif // PYBRICKS_HUB_PRIMEHUB
    { MP_ROM_QSTR(MP_QSTR_hello_world), MP_ROM_PTR(&experimental_hello_world_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_experimental_globals, experimental_globals_table);

const mp_obj_module_t pb_module_experimental = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_experimental_globals,
};

#endif // PYBRICKS_PY_EXPERIMENTAL
