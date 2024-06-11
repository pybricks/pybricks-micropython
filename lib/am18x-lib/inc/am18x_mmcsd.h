// tary, 21:11 2013/6/23

#ifndef __AM18X_MMCSD_H__
#define __AM18X_MMCSD_H__

#include "am18x_map.h"

#define MMCSD_BLOCK_SIZE		0x200

typedef struct {
	uint32_t	freq;
#define TIMEOUT_RSP_MAX			0xFFUL
	uint32_t	timeout_rsp;
#define TIMEOUT_DAT_MAX			0x3FFFFFFUL
	uint32_t	timeout_dat;
} mmcsd_conf_t;

typedef enum {
	MMCSD_CMD_F_NORSP = 0,
	MMCSD_CMD_F_RSP = BIT(1),
	MMCSD_CMD_F_SHORT = 0,
	MMCSD_CMD_F_LONG = BIT(2),
	MMCSD_CMD_F_NOCRC = 0,
	MMCSD_CMD_F_CRC = BIT(3),
	MMCSD_CMD_F_NODATA = 0,
	MMCSD_CMD_F_DATA = BIT(4),
	MMCSD_CMD_F_READ = 0,
	MMCSD_CMD_F_WRITE = BIT(5),
	MMCSD_CMD_F_NOBUSY = 0,
	MMCSD_CMD_F_BUSY = BIT(6),
} mmcsd_cflags_t;

typedef struct {
	uint8_t		index;
	uint32_t	cflags;
	uint32_t 	arg;
} mmcsd_cmd_t;

typedef enum {
	MMCSD_SC_NONE,
	MMCSD_SC_RSP_OK,
	MMCSD_SC_CRC_ERR,
	MMCSD_SC_RSP_TOUT,
} mmcsd_cmd_state_t;

typedef enum {
	MMCSD_SD_NONE,
	MMCSD_SD_SENT,
	MMCSD_SD_RECVED,
	MMCSD_SD_CRC_ERR,
	MMCSD_SD_TOUT,
	MMCSD_SD_OK,
	MMCSD_SD_BUSY,
	MMCSD_SD_DONE,
} mmcsd_dat_state_t;

typedef struct {
	uint32_t v[4];
} mmcsd_resp_t;

typedef enum {
	MMCSD_RESP_SHORT,
	MMCSD_RESP_LONG,
} mmcsd_resp_type_t;

typedef enum {
	MMCSD_MISC_F_FIFO_RST = BIT(0),
	MMCSD_MISC_F_FIFO_32B = 0,
	MMCSD_MISC_F_FIFO_64B = BIT(1),
	MMCSD_MISC_F_READ = 0,
	MMCSD_MISC_F_WRITE = BIT(2),
	MMCSD_MISC_F_BUSY = BIT(3),
	MMCSD_MISC_F_BUS4BIT = BIT(4),
} mmcsd_mflags_t;

typedef struct {
	uint16_t	mflags;
	uint16_t	blkcnt;
} mmcsd_misc_t;

am18x_rt mmcsd_con_init(MMCSD_con_t* mcon, const mmcsd_conf_t* conf);
uint32_t mmcsd_xet_freq(MMCSD_con_t* mcon, uint32_t freq);
am18x_rt mmcsd_send_cmd(MMCSD_con_t* mcon, const mmcsd_cmd_t* cmd);
mmcsd_cmd_state_t mmcsd_cmd_state(const MMCSD_con_t* mcon, am18x_bool need_crc);
mmcsd_dat_state_t mmcsd_busy_state(const MMCSD_con_t* mcon);
mmcsd_dat_state_t mmcsd_rd_state(const MMCSD_con_t* mcon);
mmcsd_dat_state_t mmcsd_wr_state(const MMCSD_con_t* mcon);
am18x_rt mmcsd_get_resp(const MMCSD_con_t* mcon, mmcsd_resp_type_t type, mmcsd_resp_t* resp);
am18x_rt mmcsd_cntl_misc(MMCSD_con_t* mcon, const mmcsd_misc_t* misc);
uint32_t mmcsd_read(const MMCSD_con_t* mcon);
am18x_rt mmcsd_write(MMCSD_con_t* mcon, uint32_t data);

#endif//__AM18X_MMCSD_H__
