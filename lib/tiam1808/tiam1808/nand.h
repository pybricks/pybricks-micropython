/**
 *  \file   nand.h
 *
 *  \brief  Definitions used for NAND
 *
 *   This file contains the nand middle layer API prototypes and macro definitions.
 */

/*
* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
*/
/*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


#ifndef _NAND_H_
#define _NAND_H__

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
*                           MACRO DEFINITIONS
*******************************************************************************/
/* Resister Location of NAND to communicate                         */

#define AM1808
#ifdef AM1808
    #define CS3_NAND_DATA_REG                    (0x62000000u)
    #define CS3_NAND_CMD_REG                     (0x62000010u)
    #define CS3_NAND_ADDR_REG                    (0x62000008u)
#endif    

/* Macro to read the Data from Data Register                        */
#define NANDDataRead    (*(volatile unsigned char*)(CS3_NAND_DATA_REG))

/* Macro to write the Address To NAND address Register              */
#define NANDAddressWrite(address) \
                        (*(volatile unsigned char*)(CS3_NAND_ADDR_REG) = (address))

/* Macro to write the Command To NAND Command Register              */
#define NANDCommandWrite(command) \
                        (*(volatile unsigned char*)(CS3_NAND_CMD_REG) = (command))

/* Macro to write the Data to Data Register                         */
#define NANDDataWrite(data) \
                        (*(volatile unsigned char*)(CS3_NAND_DATA_REG) = (data))
#define NANDDataWrite16(data) \
                        (*(volatile unsigned short*)(CS3_NAND_DATA_REG) = (data))

/* Macro to read the Data from Data Register                        */
#define NANDDataRead    (*(volatile unsigned char*)(CS3_NAND_DATA_REG))
#define NANDDataRead16    (*(volatile unsigned short*)(CS3_NAND_DATA_REG))

/* Number Of Pages Per Blk                                          */
#define NAND_PAGES_PER_BLK                       (64)
#define NAND_PAGE_SIZE                           (2048)
#define NAND_RD_WR_SIZE                          (NAND_PAGE_SIZE)
#define NAND_NUM_OF_TRNFS                        (4)
#define NAND_BYTES_PER_TRNFS                     (NAND_RD_WR_SIZE / \
                                                  NAND_NUM_OF_TRNFS)
#define NAND_SPARE_AREA_BAD_BLOCK_MARK_OFFSET    (2048)


/* NAND STATUS VALUES                                               */
#define NAND_STATUS_CMD_PASSED                   (0)
#define NAND_STATUS_CMD_FAILED                   (1)
#define NAND_STATUS_WAIT_TIMEOUT                 (2)
#define NAND_STATUS_WRITE_PROTECTED              (3)

#define NAND_1BIT_ECC                            (1)
#define NAND_4BIT_ECC                            (2)

#define NAND_ECC_CHECK_PASSED                    (0u)
#define NAND_ECC_CHECK_FAILED                    (1u)
#define NAND_ECC_CORRECTION_ECCSTATE_0           (0u)
#define NAND_ECC_CORRECTION_ECCSTATE_2           (2u)
#define NAND_ECC_CORRECTION_ECCSTATE_3           (3u)
#define NAND_ECC_CORRECTION_NO_ERROR             (0)
#define NAND_ECC_ERROR_CORRECTED                 (1)
#define NAND_ECC_CORRECTION_UNCORRECTABLE_ERROR  (2)

#define NAND_BLOCK_GOOD                          (0)
#define NAND_BLOCK_BAD                           (1)
#define NAND_BLOCK_SPARE_AREA_READ_FAILED        (2)


#define NAND_BLK_GOOD_MARK                       (0xFF)
#define NAND_BLK_BAD_MARK                        (0)

#ifdef NAND_ECC_TYPE_1BIT
    #define NAND_ECC_DATA_SIZE                   (12u)
#else
    #define NAND_ECC_DATA_SIZE                   (40u)
#endif

#define NAND_ECC_BYTES_FOR_BYTES_PER_TRNFS       (NAND_ECC_DATA_SIZE/ \
                                                  NAND_NUM_OF_TRNFS)

/***************************************************************************/
/*
** Function Prototypes
*/
extern unsigned int NANDIdRead();
extern unsigned int NANDStatusGet();
extern unsigned int NANDResetDevice();
extern unsigned int NANDWaitUntilReady();
extern unsigned int NANDPageWriteCmdEnd();
extern void NANDDelay(volatile unsigned int delay);
extern void NANDECCRead(unsigned char *ptrEccData,unsigned int eccType,
                 unsigned int csNum);
extern unsigned int NANDECCCheck(unsigned char *ptrEccData, 
                                 unsigned int eccType, unsigned int csNum);
extern unsigned int NANDBlockErase (unsigned int blkNum);
extern void NANDECCSelectAndStart(unsigned int csNum,unsigned int eccType);
extern unsigned int NANDBadBlockCheck(unsigned int blkNum);
extern unsigned int NANDMarkBlockAsBad(unsigned int blkNum);
extern unsigned int NANDECCCorrect(unsigned char * ptrData,unsigned eccType,
                                   unsigned int eccDiffVal);
extern void NANDPageWriteCmdStart(unsigned int blkNum, unsigned int pageNum, 
                                  unsigned int columnAddr);
extern unsigned int NANDPageReadCmdSend(unsigned int blkNum, 
                                        unsigned int pageNum, 
                                        unsigned int columnAddr);
extern unsigned int NANDSpareAreaWrite(unsigned int blkNum, 
                                       unsigned int pageNum, 
                                       unsigned int columnAddr,
                                       unsigned int numOfBytes,unsigned char *data);
extern unsigned int NANDSpareAreaRead(unsigned int blkNum, unsigned int pageNum, 
                                      unsigned int columnAddr, 
                                      unsigned int numOfBytes,unsigned char *data);

#ifdef __cplusplus
}
#endif
#endif
