/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2006 David Anderson <david.anderson@calixo.net> */

/**
 * NXT bootstrap interface; NXT onboard flashing driver.
 */

#define VINTPTR(addr) ((volatile unsigned int *)(addr))
#define VINT(addr) (*(VINTPTR(addr)))

#define USER_PAGE VINTPTR(0x00202100)
#define USER_PAGE_NUM VINT(0x00202300)

#define FLASH_BASE VINTPTR(0x00100000)
#define FLASH_CMD_REG VINT(0xFFFFFF64)
#define FLASH_STATUS_REG VINT(0xFFFFFF68)
#define OFFSET_PAGE_NUM ((USER_PAGE_NUM & 0x000003FF) << 8)
#define FLASH_CMD_WRITE (0x5A000001 + OFFSET_PAGE_NUM)

void do_flash_write(void)
{
  unsigned long i;

  while (!(FLASH_STATUS_REG & 0x1));

  for (i = 0; i < 64; i++)
    FLASH_BASE[(USER_PAGE_NUM*64)+i] = USER_PAGE[i];

  FLASH_CMD_REG = FLASH_CMD_WRITE;

  while (!(FLASH_STATUS_REG & 0x1));
}
