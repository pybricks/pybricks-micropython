/**
 * \file  syscfg.c
 *
 * \brief This file contains APIs to lock and unlock the System Configuration
 *        (SYSCFG) module registers by appropriately programming the Kick 
 *        Registers.
 */

/*
* Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
*
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
*/

#include "hw_syscfg0_AM1808.h"
#include "soc_AM1808.h"
#include "evmAM1808.h"
#include "hw_types.h"

/**
 * \brief   This function unlocks the write-protection of the SYSCFG module. 
 * 
 * \return  None.
 * 
 * \note    The other registers of the SYSCFG module can be programmed only 
 *          when an unlock sequence has been written to the Kick Regsiters. 
 */

void SysCfgRegistersUnlock(void)
{
     /* Unlocking Kick Registers before pinmux configuration */
     HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_KICK0R) = SYSCFG_KICK0R_UNLOCK;
     HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_KICK1R) = SYSCFG_KICK1R_UNLOCK; 

}

/**
 * \brief   This function imposes write-protection on the SYSCFG module.
 *
 * \return  None.
 *
 * \note    On programming the Kick Registers with any value other than the 
 *          unlock sequence, the SYSCFG module gets locked and its registers 
 *          cannot be accessed.
 */

void SysCfgRegistersLock(void)
{
     /* Locking the Kick Registers. */
     HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_KICK0R) = SYSCFG_KICK0R_KICK0;
     HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_KICK1R) = SYSCFG_KICK1R_KICK1;
}

/****************************** End of file *********************************/

