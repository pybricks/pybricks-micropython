#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_POSIX

#include "btstack_config.h"

#include "bluetooth_btstack.h"
#include "bluetooth_btstack_posix.h"

#include <errno.h>
#include <getopt.h>
#include <libusb.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pbio/util.h>

#include "btstack.h"
#include "ble/le_device_db_tlv.h"
#include "btstack_chipset_realtek.h"
#include "btstack_tlv_posix.h"
#include "hci.h"
#include "hci_transport_usb.h"
#include "hci_dump_posix_stdout.h"

/**
 * A USB Bluetooth dongle that this application knows how to drive.
 */
typedef struct {
    /** Human readable name, used in diagnostic messages. */
    const char *name;
    /** USB vendor ID. */
    uint16_t usb_vendor_id;
    /** USB product ID. */
    uint16_t usb_product_id;
    /** Bluetooth company ID reported in the HCI local version information. */
    uint16_t bluetooth_company_id;
    /** Selects the BTstack chipset driver, if the dongle needs one. */
    void (*set_chipset)(const pbdrv_bluetooth_btstack_local_version_info_t *info);
} pbdrv_bluetooth_usb_dongle_t;

#define TLV_DB_PATH_PREFIX "/tmp/btstack_"
#define TLV_DB_PATH_POSTFIX ".tlv"
static char tlv_db_path[100];
static const btstack_tlv_t *tlv_impl;
static btstack_tlv_posix_t tlv_context;
static bd_addr_t local_addr;

// Set by BTstack's packet handler when we get USB info and local version info.
static uint16_t usb_product_id;
static uint16_t usb_vendor_id;

// The dongle selected during platform init.
static const pbdrv_bluetooth_usb_dongle_t *dongle;

static const pbdrv_bluetooth_btstack_chipset_info_t usb_chipset_info = {
    .supports_ble = true,
};

#define RTL_FIRMWARE_PATH "/lib/firmware/rtl_bt/rtl8761bu_fw.bin"
#define RTL_CONFIG_PATH "/lib/firmware/rtl_bt/rtl8761bu_config.bin"

/**
 * Sets up the Realtek RTL8761BU in the TP-Link UB500, which needs the host to
 * upload its firmware and config.
 */
static void set_chipset_realtek(const pbdrv_bluetooth_btstack_local_version_info_t *info) {

    btstack_chipset_realtek_set_lmp_subversion(info->lmp_pal_subversion);
    btstack_chipset_realtek_set_product_id(usb_product_id);

    if (access(RTL_FIRMWARE_PATH, R_OK) != 0 || access(RTL_CONFIG_PATH, R_OK) != 0) {
        // Newer distros ship these zstd-compressed. So run one off:
        // sudo zstd -dk /lib/firmware/rtl_bt/rtl8761bu_fw.bin.zst /lib/firmware/rtl_bt/rtl8761bu_config.bin.zst
        printf("Realtek firmware not found");
    }

    btstack_chipset_realtek_set_firmware_file_path(RTL_FIRMWARE_PATH);
    btstack_chipset_realtek_set_config_file_path(RTL_CONFIG_PATH);

    hci_set_chipset(btstack_chipset_realtek_instance());
}

/**
 * Sets up the CSR8510 A10 found in the common Cambridge Silicon Radio dongles.
 *
 * The BTstack CSR chipset driver only exists to configure PSKEYs over UART
 * transports, so on USB the dongle is ready to use as-is.
 */
static void set_chipset_csr(const pbdrv_bluetooth_btstack_local_version_info_t *info) {
    (void)info;
}

static const pbdrv_bluetooth_usb_dongle_t supported_dongles[] = {
    {
        .name = "TP-Link UB500",
        .usb_vendor_id = 0x2357,
        .usb_product_id = 0x0604,
        .bluetooth_company_id = BLUETOOTH_COMPANY_ID_REALTEK_SEMICONDUCTOR_CORPORATION,
        .set_chipset = set_chipset_realtek,
    },
    {
        .name = "CSR8510 A10",
        .usb_vendor_id = 0x0a12,
        .usb_product_id = 0x0001,
        .bluetooth_company_id = BLUETOOTH_COMPANY_ID_CAMBRIDGE_SILICON_RADIO,
        .set_chipset = set_chipset_csr,
    },
};

/**
 * Looks up a supported dongle by its USB IDs.
 *
 * @return The matching dongle, or @c NULL if it is not one we support.
 */
static const pbdrv_bluetooth_usb_dongle_t *find_dongle(uint16_t vendor_id, uint16_t product_id) {
    for (size_t i = 0; i < PBIO_ARRAY_SIZE(supported_dongles); i++) {
        const pbdrv_bluetooth_usb_dongle_t *candidate = &supported_dongles[i];
        if (candidate->usb_vendor_id == vendor_id && candidate->usb_product_id == product_id) {
            return candidate;
        }
    }
    return NULL;
}

// We should not get here since we filtered the device earlier, but this
// asserts that BTstack has discovered the same device from the port ID.
static void assert_selected_dongle(uint16_t vendor_id, uint16_t product_id) {
    if (!dongle || vendor_id != dongle->usb_vendor_id || product_id != dongle->usb_product_id) {
        printf("Unexpected USB device: vendor ID 0x%04x, product ID 0x%04x\n", vendor_id, product_id);
        exit(1);
    }
}

// As above, but for the Bluetooth company ID from the HCI local version info.
static void assert_manufacturer_id(uint16_t manufacturer_id) {
    if (manufacturer_id != dongle->bluetooth_company_id) {
        printf("Unexpected Bluetooth manufacturer ID: 0x%04x\n", manufacturer_id);
        exit(1);
    }
}

const pbdrv_bluetooth_btstack_chipset_info_t *pbdrv_bluetooth_btstack_set_chipset(pbdrv_bluetooth_btstack_local_version_info_t *device_info) {

    assert_manufacturer_id(device_info->manufacturer);
    assert_selected_dongle(usb_vendor_id, usb_product_id);

    dongle->set_chipset(device_info);

    return &usb_chipset_info;
}

static void noop_voidstararg(const void *) {
}

static int noop_returnint(void) {
    return 0;
}

static const btstack_control_t noop_btstack_control = {
    .init = noop_voidstararg,
    .on = noop_returnint,
    .off = noop_returnint,
    .sleep = noop_returnint,
    .wake = noop_returnint,
    .register_for_power_notifications = NULL,
};

const btstack_control_t *pbdrv_bluetooth_btstack_posix_control_instance(void) {
    return &noop_btstack_control;
}

const hci_transport_t *pbdrv_bluetooth_btstack_posix_transport_instance(void) {
    return hci_transport_usb_instance();
}

const void *pbdrv_bluetooth_btstack_posix_transport_config(void) {
    return NULL;
}

/**
 * Attempts to find the specified USB Bluetooth dongle among connected devices.
 *
 * BTstack has several ways to specify which USB device to use, but none are
 * suitable for our use case: we need to be able to specify multiple identical
 * devices on the same system.
 *
 * This function uses libusb to find the correct device based on an index
 * specified in the USB_BLE_INDEX environment variable, and then passes the
 * port numbers to BTstack. The index only serves to consistently pick the same
 * dongle across runs; which dongle gets which index may change when devices
 * are replugged or the system is rebooted.
 *
 * @return ::PBIO_SUCCESS on success, or an ::ERROR_NO_DEV error if the device
 *         could not be found, so it can continue without Bluetooth.
 */
pbio_error_t pbdrv_bluetooth_btstack_platform_init(void) {

    const char *env_index = getenv("USB_BLE_INDEX");
    if (!env_index) {
        // Silently continue without Bluetooth if not specified.
        return PBIO_ERROR_NO_DEV;
    }

    printf("Looking for USB Bluetooth dongle with index %s.\n", env_index);

    int target_index = atoi(env_index);

    libusb_context *ctx = NULL;
    libusb_device **list = NULL;

    if (libusb_init(&ctx) < 0) {
        return PBIO_ERROR_NO_DEV;
    }

    ssize_t count = libusb_get_device_list(ctx, &list);
    if (count < 0) {
        libusb_exit(ctx);
        return PBIO_ERROR_NO_DEV;
    }

    libusb_device *match = NULL;
    int match_count = 0;

    for (ssize_t i = 0; i < count; i++) {
        libusb_device *dev = list[i];
        struct libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(dev, &desc) != 0) {
            continue;
        }
        const pbdrv_bluetooth_usb_dongle_t *candidate = find_dongle(desc.idVendor, desc.idProduct);
        if (candidate) {
            if (match_count == target_index) {
                match = dev;
                dongle = candidate;
                break;
            }
            match_count++;
        }
    }

    pbio_error_t err = PBIO_SUCCESS;

    if (!match) {
        err = PBIO_ERROR_NO_DEV;
        goto exit;
    }

    printf("USB device found: %s.\n", dongle->name);

    uint8_t ports[16];
    int port_count = libusb_get_port_numbers(match, ports, sizeof(ports));

    if (port_count) {
        // Tell BTstack to use this port path.
        hci_transport_usb_set_path(port_count, ports);
        printf("Using port: ");
        for (int i = 0; i < port_count; i++) {
            printf("%u%s", ports[i], i == port_count - 1 ? "" : ".");
        }
        printf("\n");
    } else {
        err = PBIO_ERROR_NO_DEV;
    }

exit:
    libusb_free_device_list(list, 1);
    libusb_exit(ctx);

    if (err != PBIO_SUCCESS) {
        printf("Could not find specified device or port.\n");
    }
    return err;
}

void pbdrv_bluetooth_btstack_platform_poll(void) {

    btstack_run_loop_base_poll_data_sources();

    int nfds = btstack_linked_list_count(&btstack_run_loop_base_data_sources);
    struct pollfd fds[nfds];

    btstack_linked_list_iterator_t it;
    int i;
    for (i = 0, btstack_linked_list_iterator_init(&it, &btstack_run_loop_base_data_sources);
         btstack_linked_list_iterator_has_next(&it); ++i) {
        // cache pointer to next data_source to allow data source to remove itself
        btstack_data_source_t *ds = (void *)btstack_linked_list_iterator_next(&it);

        // Identify data source FDs that are ready for reading or writing.
        struct pollfd *pfd = &fds[i];
        pfd->fd = ds->source.fd;
        pfd->events = 0;
        if (ds->flags & DATA_SOURCE_CALLBACK_READ) {
            pfd->events |= POLLIN;
        }
        if (ds->flags & DATA_SOURCE_CALLBACK_WRITE) {
            pfd->events |= POLLOUT;
        }

    }

    int err = poll(fds, nfds, 0);
    if (err < 0) {
        printf("btstack: poll() returned %d, ignoring\n", errno);
    } else if (err > 0) {
        // Some fd was ready.
        for (i = 0, btstack_linked_list_iterator_init(&it, &btstack_run_loop_base_data_sources);
             btstack_linked_list_iterator_has_next(&it); ++i) {
            btstack_data_source_t *ds = (void *)btstack_linked_list_iterator_next(&it);
            struct pollfd *pfd = &fds[i];
            if (pfd->revents & POLLIN) {
                ds->process(ds, DATA_SOURCE_CALLBACK_READ);
            } else if (pfd->revents & POLLOUT) {
                ds->process(ds, DATA_SOURCE_CALLBACK_WRITE);
            } else if (pfd->revents & POLLERR) {
                printf("btstack: poll() error on fd %d\n", pfd->fd);
            }
        }
    }
}

void pbdrv_bluetooth_btstack_platform_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_TRANSPORT_USB_INFO: {
            usb_vendor_id = hci_event_transport_usb_info_get_vendor_id(packet);
            usb_product_id = hci_event_transport_usb_info_get_product_id(packet);
            assert_selected_dongle(usb_vendor_id, usb_product_id);
            break;
        }
        case BTSTACK_EVENT_STATE:
            switch (btstack_event_state_get_state(packet)) {
                case HCI_STATE_WORKING:
                    gap_local_bd_addr(local_addr);
                    btstack_strcpy(tlv_db_path, sizeof(tlv_db_path), TLV_DB_PATH_PREFIX);
                    btstack_strcat(tlv_db_path, sizeof(tlv_db_path), bd_addr_to_str_with_delimiter(local_addr, '-'));
                    btstack_strcat(tlv_db_path, sizeof(tlv_db_path), TLV_DB_PATH_POSTFIX);
                    printf("\n");
                    tlv_impl = btstack_tlv_posix_init_instance(&tlv_context, tlv_db_path);
                    btstack_tlv_set_instance(tlv_impl, &tlv_context);
                    // NB: Classic link keys are persisted in pbsys storage
                    // via the link key db set by the main btstack driver.
                    #ifdef ENABLE_BLE
                    le_device_db_tlv_configure(tlv_impl, &tlv_context);
                    #endif
                    printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
                    break;
                case HCI_STATE_OFF:
                    btstack_tlv_posix_deinit(&tlv_context);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_POSIX
