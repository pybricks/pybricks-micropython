import argparse

# Expected layout of the firmware image:
#
# u-boot.bin can be the first 256KiB from official firmware or a custom U-Boot.
#
# | Image (file)       | Start Addr. | Max. Size         | Notes
# +--------------------+-------------+-------------------+----------
# | u-boot.bin         |         0x0 |  0x40000 (256KiB) |
# | da850-lego-ev3.dtb |     0x40000 |  0x10000 (64KiB)  | Not used.
# | uImage             |     0x50000 | 0x400000 (4MiB)   | Smaller size is allowed.
# | rootfs (squashfs)  |    0x450000 | 0xa00000 (10MiB)  | Not used.


UBOOT_MAX_SIZE = 256 * 1024
UIMAGE_MAX_SIZE = 4 * 1024 * 1024
UIMAGE_OFFSET = 0x50000
FIRMWARE_ROUND_SIZE = 256 * 1024


def make_firmware(uboot_blob, uimage_blob):

    if len(uboot_blob) > UBOOT_MAX_SIZE:
        print("u-boot file is bigger than expected. Using only the first 256KiB.")
        uboot_blob = uboot_blob[:UBOOT_MAX_SIZE]

    if len(uimage_blob) > UIMAGE_MAX_SIZE:
        raise ValueError("uImage file is too big.")

    # Gets combined size, rounded to nearest expected size.
    combined_size = UIMAGE_OFFSET + len(uimage_blob)
    combined_size = (
        (combined_size + FIRMWARE_ROUND_SIZE) // FIRMWARE_ROUND_SIZE * FIRMWARE_ROUND_SIZE
    )

    # Put it all together.
    padding_uboot = b"\0" * (UIMAGE_OFFSET - len(uboot_blob))
    padding_uimage = b"\0" * (combined_size - UIMAGE_OFFSET - len(uimage_blob))
    return uboot_blob + padding_uboot + uimage_blob + padding_uimage


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Create a bootable EV3 firmware image from a u-boot and uImage file."
    )
    parser.add_argument(
        "uboot",
        help="The u-boot file, or an official firmware file from which to extract the u-boot.",
    )
    parser.add_argument("uimage", help="The uImage file")
    parser.add_argument("output", help="The output file")
    args = parser.parse_args()

    with open(args.uboot, "rb") as uboot_file, open(args.uimage, "rb") as uimage_file, open(
        args.output, "wb"
    ) as output_file:
        combined = make_firmware(uboot_file.read(), uimage_file.read())
        output_file.write(combined)
        print(f"Created {args.output} with size {len(combined) // 1024} KB.")
