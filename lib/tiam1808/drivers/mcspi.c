/**
*
*  \file mcspi.c
*
*  \brief McSPI device abstraction layer APIs
**/

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

#include "mcspi.h"
#include "hw_types.h"

/*******************************************************************************
*                       INTERNAL MACRO DEFINITIONS
*******************************************************************************/
#define MCSPI_CH_NUM_0        (0u)
#define MCSPI_CLKD_MASK       (0xF)

/*******************************************************************************
*                        API FUNCTION DEFINITIONS
*******************************************************************************/

/**
* \brief  This API will configure the clkD and extClk fields to generate
*         required spi clock depending on the type of granularity. It will
*         also set the phase and polarity of spiClk by the clkMode field.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  spiInClk       Clock frequency given to the McSPI module.
* \param  spiOutClk      Clock frequency on the McSPI bus.
* \param  chNum          Channel number of the McSPI instance used.
* \param  clkMode        Clock mode used.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
*         'clkMode' can take the following values.\n
*         MCSPI_CLK_MODE_n - McSPI clock mode n.\n
*
*         For clkMode 0 <= n <= 3.\n
*
* \return none.
*
* \note:  1) clkMode depends on phase and polarity of McSPI clock.\n
*         2) To pass the desired value for clkMode please refer the
*            McSPI_CH(i)CONF register.\n
*         3) Please understand the polarity and phase of the slave device
*            connected and accordingly set the clkMode.\n
*         4) McSPIClkConfig does not have any significance in slave mode
*            because the clock signal required for communication is generated
*            by the master device.
**/
void McSPIClkConfig(unsigned int baseAdd, unsigned int spiInClk,
                    unsigned int spiOutClk, unsigned int chNum,
                    unsigned int clkMode)
{
    unsigned int fRatio = 0;
    unsigned int extClk = 0;
    unsigned int clkD = 0;

    /* Calculate the value of fRatio. */
    fRatio = (spiInClk / spiOutClk);

    /* If fRatio is not a power of 2, set granularity of 1 clock cycle */
    if(0 != (fRatio & (fRatio - 1)))
    {
        /* Set the clock granularity to 1 clock cycle.*/
        HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= MCSPI_CH0CONF_CLKG;

        /* Calculate the ratios clkD and extClk based on fClk */
        extClk = (fRatio - 1) >> 4;
        clkD = (fRatio - 1) & MCSPI_CLKD_MASK;

        /*Clear the extClk field of MCSPI_CHCTRL register.*/
        HWREG(baseAdd + MCSPI_CHCTRL(chNum)) &= ~MCSPI_CH0CTRL_EXTCLK;

        /* Set the extClk field of MCSPI_CHCTRL register.*/
        HWREG(baseAdd + MCSPI_CHCTRL(chNum)) |= (extClk <<
                                                 MCSPI_CH0CTRL_EXTCLK_SHIFT);
    }
    else
    {
        /* Clock granularity of power of 2.*/
        HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_CLKG;

        while(1 != fRatio)
        {
            fRatio /= 2;
            clkD++;
        }
    }

    /*Clearing the clkD field of MCSPI_CHCONF register.*/
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_CLKD;

    /* Configure the clkD field of MCSPI_CHCONF register.*/
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= (clkD << MCSPI_CH0CONF_CLKD_SHIFT);

    /*Clearing the clkMode field of MCSPI_CHCONF register.*/
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~(MCSPI_CH0CONF_PHA |
                                               MCSPI_CH0CONF_POL);

    /* Configure the clkMode of MCSPI_CHCONF register.*/
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= (clkMode & (MCSPI_CH0CONF_PHA |
                                                         MCSPI_CH0CONF_POL));
}

/**
* \brief  This API will configure the length of McSPI word used for
*         communication.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  wordLength     Length of a data word used for McSPI communication.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'wordLength' can take the following values.\n
*          MCSPI_WORD_LENGTH(n)  - McSPI word length is n bits long.\n
*
*          For wordLength 4 <= n <= 32.\n
*
*          'chNum' can take the following values.\n
*          MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*          For chNum n can vary from 0-3.\n
*
* \return none.
*
* \note:  wordLength can vary from 4-32 bits length. To program the
*         required value of wordLength please refer the MCSPI_CH(i)CONF
*         register.\n
**/
void McSPIWordLengthSet(unsigned int baseAdd, unsigned int wordLength,
                        unsigned int chNum)
{
    /*Clearing the wordLength field of MCSPI_CHCONF register.*/
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_WL;

    /* Setting the wordlength field. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= (wordLength & MCSPI_CH0CONF_WL);
}

/**
* \brief  This API will enable the chip select pin.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
*
* \return none.
*
* \note:  Modification of CS polarity, SPI clock phase and polarity
          is not allowed when CS is enabled.
**/
void McSPICSEnable(unsigned int baseAdd)
{
    /* CLear PIN34 of MCSPI_MODULCTRL register. */
    HWREG(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_PIN34;
}

/**
* \brief  This API will disable the chip select pin.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
*
* \return none.
**/
void McSPICSDisable(unsigned int baseAdd)
{
    /* Set PIN34 of MCSPI_MODULCTRL register.*/
    HWREG(baseAdd + MCSPI_MODULCTRL) |= MCSPI_MODULCTRL_PIN34;
}

/**
* \brief  This API will configure the chip select polarity.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  spiEnPol       Polarity of CS line.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*        'spiEnPol' can take the following values.\n
*         MCSPI_CS_POL_HIGH - SPIEN pin is held high during the active state.\n
*         MCSPI_CS_POL_LOW - SPIEN pin is held low during the active state.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return none.
**/
void McSPICSPolarityConfig(unsigned int baseAdd, unsigned int spiEnPol,
                           unsigned int chNum)
{
    /* Clear the EPOL field of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_EPOL;

    /* Set the EPOL field with the user sent value. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= (spiEnPol & MCSPI_CH0CONF_EPOL);
}

/**
* \brief  This API will configure the chip select time control.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  csTimeControl  Chip Select time control.
* \param  chNum          Channel number of the McSPI instance used.
*
*         'csTimeControl' can take the following values.\n
*         MCSPI_CS_TCS_nPNT5_CLK - n.5 Clock cycles.\n
*
*         For csTimeControl 0 <= n <= 3.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return none.
**/
void McSPICSTimeControlSet(unsigned int baseAdd, unsigned int csTimeControl,
                           unsigned int chNum)
{
    /* Clear the TCS field of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_TCS0;

    /* Set the TCS field with the user sent value. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= (csTimeControl &
                                              MCSPI_CH0CONF_TCS0);
}

/**
* \brief  This API will activate the chip select line.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  chNum          Channel number of the McSPI instance used.
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return none.
**/
void McSPICSAssert(unsigned int baseAdd, unsigned int chNum)
{
    /* Set the FORCE bit of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= (MCSPI_CH0CONF_FORCE);
}

/**
* \brief  This API will deactivate the chip select line.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  chNum          Channel number of the McSPI instance used.
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return none.
**/
void McSPICSDeAssert(unsigned int baseAdd, unsigned int chNum)
{
    /* Clear the FORCE bit of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~(MCSPI_CH0CONF_FORCE);
}

/**
* \brief  This API will enable start bit.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return none.
**/
void McSPIStartBitEnable(unsigned int baseAdd, unsigned int chNum)
{
    /* Set the SBE bit of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= MCSPI_CH0CONF_SBE;
}

/**
* \brief  This API will configure the polarity of start bit.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  startBitPol    Polarity of start bit.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'startBitPol' can take the following values.\n
*         MCSPI_START_BIT_POL_LOW  - Polarity of start bit is held low
*                                    during transmission.\n
*         MCSPI_START_BIT_POL_HIGH - Polarity of start bit is held high
*                                    during transmission.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return none.
**/
void McSPIStartBitPolarityConfig(unsigned int baseAdd, unsigned int startBitPol,
                                 unsigned int chNum)
{
    /* Clear the SBE field of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_SBPOL;

    /* Set the user sent value */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= (startBitPol &
                                              MCSPI_CH0CONF_SBPOL);
}

/**
* \brief  This API will disable the start bit mode of McSPI peripheral.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return none.
**/
void McSPIStartBitDisable(unsigned int baseAdd, unsigned int chNum)
{
    /* Clear the SBE field of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_SBE;
}

/**
* \brief  This API will enable the McSPI controller in master mode.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
*
* \return none.
**/
void McSPIMasterModeEnable(unsigned int baseAdd)
{
    /* Clear the MS field of MCSPI_MODULCTRL register. */
    HWREG(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_MS;
}

/**
* \brief  This API will enable the McSPI controller in slave mode.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
*
* \return none.
**/
void McSPISlaveModeEnable(unsigned int baseAdd)
{
    /* Set the MS field of MCSPI_MODULCTRL register. */
    HWREG(baseAdd + MCSPI_MODULCTRL) |= MCSPI_MODULCTRL_MS;
}

/**
* \brief  This API will enable the McSPI controller in master mode and
*         configure other parameters required for master mode.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  channelMode    Single/Multi channel.
* \param  trMode         Transmit/Receive mode used in master
*                        configuration.
* \param  pinMode        Interface mode and pin assignment.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'channelMode' can take the following values.\n
*         MCSPI_SINGLE_CH - Single channel mode is used.\n
*         MCSPI_MULTI_CH  - Multi channel mode is used.\n
*
*         'trMode' can take the following values.\n
*         MCSPI_TX_RX_MODE   - Enable McSPI in TX and RX modes.\n
*         MCSPI_RX_ONLY_MODE - Enable McSPI only in RX mode.\n
*         MCSPI_TX_ONLY_MODE - Enable McSPI only in TX mode.\n
*
*         'pinMode' can take the following values.\n
*         MCSPI_DATA_LINE_COMM_MODE_n - Mode n configuration for SPIDAT[1:0].\n
*
*         For pinMode 0 <= n <= 7.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return 'retVal' which states if the combination of trMode and pinMode chosen
*          by the user is supported for communication on SPIDAT[1:0] pins.\n
*          TRUE  - Communication supported by SPIDAT[1:0].\n
*          FALSE - Communication not supported by SPIDAT[1:0].\n
*
* \note   Please refer the description about IS,DPE1,DPE0 and TRM bits for
*         proper configuration of SPIDAT[1:0].\n
**/
unsigned int McSPIMasterModeConfig(unsigned int baseAdd,
                                   unsigned int channelMode,
                                   unsigned int trMode,
                                   unsigned int pinMode,
                                   unsigned int chNum)
{
    unsigned int retVal = FALSE;

    /* Clear the MS field of MCSPI_MODULCTRL register. */
    HWREG(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_SINGLE;

    /* Set the MS field with the user sent value. */
    HWREG(baseAdd + MCSPI_MODULCTRL) |= (channelMode & MCSPI_MODULCTRL_SINGLE);

    /* Clear the TRM field of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_TRM;

    /* Set the TRM field with the user sent value. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= (trMode & MCSPI_CH0CONF_TRM);

    if(((MCSPI_TX_RX_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_3 == pinMode)) ||
       ((MCSPI_TX_ONLY_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_3 == pinMode)) ||
       ((MCSPI_TX_RX_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_7 == pinMode)) ||
       ((MCSPI_TX_ONLY_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_7 == pinMode)))
    {
        retVal = FALSE;
    }
    else
    {
        /* Clear the IS, DPE0, DPE1 fields of MCSPI_CHCONF register. */
        HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~(MCSPI_CH0CONF_IS |
                                                   MCSPI_CH0CONF_DPE1 |
                                                   MCSPI_CH0CONF_DPE0);

        /* Set the IS, DPE0, DPE1 fields with the user sent values. */
        HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= (pinMode & (MCSPI_CH0CONF_IS |
                                                            MCSPI_CH0CONF_DPE1 |
                                                            MCSPI_CH0CONF_DPE0));

        retVal = TRUE;
    }

    return retVal;
}

/**
* \brief  This API will enable the McSPI controller in slave mode of
*         operation and certain configurations required for slave mode.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  trMode         Transmit/Receive mode used in slave
*                        configuration.
* \param  pinMode        Interface mode and pin assignment.\n
*
*         'trMode' can take the following values.\n
*         MCSPI_TX_RX_MODE   - Enable McSPI in TX and RX modes.\n
*         MCSPI_RX_ONLY_MODE - Enable McSPI only in RX mode.\n
*         MCSPI_TX_ONLY_MODE - Enable McSPI only in TX mode.\n
*
*         'pinMode' can take the following values.\n
*         MCSPI_DATA_LINE_COMM_MODE_n - Mode n configuration for SPIDAT[1:0].\n
*
*         For pinMode 0 <= n <= 7.\n
*
* \return 'retVal' which states if the combination of trMode and pinMode chosen
*         by the user are supported for communication on SPIDAT[1:0] pins.\n
*         TRUE  - Communication supported by SPIDAT[1:0].\n
*         FALSE - Communication not supported by SPIDAT[1:0].\n
*
* \note   Please refer to the description about IS,DPE1,DPE0 and TRM bits for
*         proper configuration of SPIDAT[1:0].\n
*         Also ensure to use only channel 0 for communication in
*         slave mode. Communication using channels other than zero
*         is not valid in slave mode.\n
**/
unsigned int McSPISlaveModeConfig(unsigned int baseAdd, unsigned int trMode,
                                  unsigned int pinMode)
{
    unsigned int retVal = TRUE;

    /* Clear the TRM field of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(MCSPI_CH_NUM_0)) &= ~MCSPI_CH0CONF_TRM;

    /* Set the TRM field with the user sent value. */
    HWREG(baseAdd + MCSPI_CHCONF(MCSPI_CH_NUM_0)) |= (trMode & MCSPI_CH0CONF_TRM);

    if(((MCSPI_TX_RX_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_3 == pinMode)) ||
       ((MCSPI_TX_ONLY_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_3 == pinMode)) ||
       ((MCSPI_TX_RX_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_7 == pinMode)) ||
       ((MCSPI_TX_ONLY_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_7 == pinMode)))
    {
        retVal = FALSE;
    }
    else
    {
        /*Clear the IS, DPE0, DPE1 fields of MCSPI_CHCONF register. */
        HWREG(baseAdd + MCSPI_CHCONF(MCSPI_CH_NUM_0)) &= ~(MCSPI_CH0CONF_IS |
                                                            MCSPI_CH0CONF_DPE1 |
                                                            MCSPI_CH0CONF_DPE0);

        /*Set the IS, DPE0, DPE1 fields with the user sent values. */
        HWREG(baseAdd + MCSPI_CHCONF(MCSPI_CH_NUM_0)) |= (pinMode &
                                                          (MCSPI_CH0CONF_IS |
                                                           MCSPI_CH0CONF_DPE1 |
                                                           MCSPI_CH0CONF_DPE0));
    }

    return retVal;
}

/**
* \brief  This API will enable the channel of McSPI controller.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*      'chNum' can take the following values.\n
*      MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*      For chNum 0 <= n <= 3.\n
*
* \return none.
*
* \note:  Please ensure to enable only channel 0 in slave mode.\n
*         Channels other than 0 are not valid in slave mode.
**/
void McSPIChannelEnable(unsigned int baseAdd, unsigned int chNum)
{
    /* Set the EN field of MCSPI_CHCTRL register. */
    HWREG(baseAdd + MCSPI_CHCTRL(chNum)) |= MCSPI_CH0CTRL_EN_ACTIVE;
}

/**
* \brief  This API will disable the channel of McSPI controller.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  chNum          Channel number of the McSPI instance used.
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return none.
**/
void McSPIChannelDisable(unsigned int baseAdd, unsigned int chNum)
{
    /* Clear the EN field of MCSPI_CHCTRL register. */
    HWREG(baseAdd + MCSPI_CHCTRL(chNum)) &= ~MCSPI_CH0CTRL_EN_ACTIVE;
}

/**
* \brief  This API will reset the McSPI peripheral.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
*
* \return none.
**/
void McSPIReset(unsigned int baseAdd)
{
    /* Set the SOFTRESET field of MCSPI_SYSCONFIG register. */
    HWREG(baseAdd + MCSPI_SYSCONFIG) |= MCSPI_SYSCONFIG_SOFTRESET;

    /* Stay in the loop until reset is done. */
    while(!(MCSPI_SYSSTATUS_RESETDONE_COMPLETED &
            HWREG(baseAdd + MCSPI_SYSSTATUS)));
}

/**
* \brief  This API will enable the McSPI turbo mode of operation.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  chNum          Channel number of the McSPI instance used.
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return none.
*
* \note
**/
void McSPITurboModeEnable(unsigned int baseAdd, unsigned int chNum)
{
    /* Set the TURBO field on MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= MCSPI_CH0CONF_TURBO;
}

/**
* \brief  This API will disable the McSPI turbo mode of operation.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  chNum          Channel number of the McSPI instance used.
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return none.
**/
void McSPITurboModeDisable(unsigned int baseAdd, unsigned int chNum)
{
    /* Clear the TURBO field of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_TURBO;
}

/**
* \brief  This API will configure the SPIDAT[1:0] pins for transmission
*         or reception.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  datDirection   Will enable either of the data pins as i/p or o/p.\n
*
*      'datDirection' can take the following values.\n
*      MCSPI_DAT0_OUT_DAT1_IN - SPIDAT[0] used as o/p pin and SPIDAT[1]
*                               used as i/p.\n
*      MCSPI_DAT1_OUT_DAT0_IN - SPIDAT[1] used as o/p pin and SPIDAT[0]
*                               used as i/p.\n
*
* \return none.
**/
void McSPIDataPinDirectionConfig(unsigned int baseAdd,
                                 unsigned int datDirection)
{
    /* Clear the SPIDATDIR1 and SPIDATDIR0 fields of MCSPI_SYST register. */
    HWREG(baseAdd + MCSPI_SYST) &= ~(MCSPI_SYST_SPIDATDIR1 |
                                     MCSPI_SYST_SPIDATDIR0);
    /* Set the SPIDATDIR1 and SPIDATDIR0 fields with the user sent value. */
    HWREG(baseAdd + MCSPI_SYST) |= (datDirection & (MCSPI_SYST_SPIDATDIR0 |
                                                    MCSPI_SYST_SPIDATDIR1));
}

/**
* \brief  This API will enable/disable the Tx FIFOs of McSPI peripheral.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  txFifo         FIFO used for transmit mode.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'txFifo' can take the following values.\n
*         MCSPI_TX_FIFO_ENABLE  - Enables the transmitter FIFO of McSPI.\n
*         MCSPI_TX_FIFO_DISABLE - Disables the transmitter FIFO of McSPI.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return none.
*
* \note:  Enabling FIFO is restricted to only 1 channel.
**/
void McSPITxFIFOConfig(unsigned int baseAdd, unsigned int txFifo,
                       unsigned int chNum)
{
    /* Clear the FFEW field of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_FFEW;

    /* Set the FFEW field with user sent value. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= (txFifo & MCSPI_CH0CONF_FFEW);
}

/**
* \brief  This API will enable/disable the Rx FIFOs of McSPI peripheral.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  rxFifo         FIFO used for receive mode.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'rxFifo' can take the following values.\n
*         MCSPI_RX_FIFO_ENABLE - Enables the receiver FIFO of McSPI.\n
*         MCSPI_RX_FIFO_DISABLE - Disables the receiver FIFO of McSPI.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return none.
*
* \note:  Enabling FIFO is restricted to only 1 channel.
**/
void McSPIRxFIFOConfig(unsigned int baseAdd, unsigned int rxFifo,
                       unsigned int chNum)
{
    /* Clear the FFER field of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_FFER;

    /* Set the FFER field with the user sent value. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= (rxFifo & MCSPI_CH0CONF_FFER);
}

/**
* \brief  This API will set the transfer levels used by FIFO depending on
*         the various McSPI transmit/receive modes.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  afl            Buffer almost full value.
* \param  ael            Buffer almost empty value.
* \param  trMode         Transmit/Receive modes used.\n
*
*         'trMode' can take the following values.\n
*         MCSPI_TX_RX_MODE   - Enable McSPI in TX and RX modes.\n
*         MCSPI_RX_ONLY_MODE - Enable McSPI only in RX mode.\n
*         MCSPI_TX_ONLY_MODE - Enable McSPI only in TX mode.\n
*
* \return none.
*
* \note   Values for afl and ael will have varying values depending on
*         trMode. If trMode is MCSPI_TX_RX_MODE then afl and ael can take
*         values ranging from 0-19. If trMode is MCSPI_RX_ONLY_MODE/
*         MCSPI_TX_ONLY_MODE then afl and ael can take values from 0-39.\n
*
*         While configuring mode of operation using trMode please ensure the
*         same value of trMode is used while using API's McSPIMasterModeConfig
*         and McSPISlaveModeConfig. Mismatch while using trMode for different
*         APIs can result in unpredictable behaviour.\n
*
*         For 'ael' and 'afl' please send level values for both of the fields
*         and do not send the value which has to be written into the register
*         for the corresponding level value.
*         For e.g. if 'ael' or 'afl' has to be 1 byte, then pass the parameter
*         'ael' or 'afl' as 1 and not 0 because the value which has to be
*         written into register is manipulated inside the driver itself.\n
**/
void McSPIFIFOTrigLvlSet(unsigned int baseAdd, unsigned char afl,
                         unsigned char ael, unsigned int trMode)
{
    if(MCSPI_TX_RX_MODE == trMode)
    {
        /* Clear the AFL and AEL fields of MCSPI_XFERLEVEL register. */
        HWREG(baseAdd + MCSPI_XFERLEVEL) &= ~(MCSPI_XFERLEVEL_AFL |
                                              MCSPI_XFERLEVEL_AEL);

        /* Set the AFL and AEL fields with the user sent value. */
        HWREG(baseAdd + MCSPI_XFERLEVEL) |= ((((afl-1) <<
                                             MCSPI_XFERLEVEL_AFL_SHIFT) &
                                             MCSPI_XFERLEVEL_AFL) |
                                             ((ael - 1) & MCSPI_XFERLEVEL_AEL));
    }
    else if(MCSPI_TX_ONLY_MODE == trMode)
    {
        /* Clear the AEL field of MCSPI_XFERLEVEL register. */
        HWREG(baseAdd + MCSPI_XFERLEVEL) &= ~MCSPI_XFERLEVEL_AEL;

        /* Set the AEL field with the user sent value. */
        HWREG(baseAdd + MCSPI_XFERLEVEL) |= ((ael - 1) & MCSPI_XFERLEVEL_AEL);
    }
    else if(MCSPI_RX_ONLY_MODE == trMode)
    {
        /* Clear the AFL field of MCSPI_XFERLEVEL register. */
        HWREG(baseAdd + MCSPI_XFERLEVEL) &= ~MCSPI_XFERLEVEL_AFL;

        /* Set the AFL field with the user sent value. */
        HWREG(baseAdd + MCSPI_XFERLEVEL) |= (((afl - 1) <<
                                               MCSPI_XFERLEVEL_AFL_SHIFT) &
                                               MCSPI_XFERLEVEL_AFL);
    }
    else
    {

    }
}

/**
* \brief  This API will set the McSPI word counter value.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  wCnt           Word count.
*
* \return none.
**/
void McSPIWordCountSet(unsigned int baseAdd, unsigned short wCnt)
{
    /* Clear the WCNT field of MCSPI_XFERLEVEL register. */
    HWREG(baseAdd + MCSPI_XFERLEVEL) &= ~MCSPI_XFERLEVEL_WCNT;

    /* Set the WCNT field with the user sent value. */
    HWREG(baseAdd + MCSPI_XFERLEVEL) |= (wCnt << MCSPI_XFERLEVEL_WCNT_SHIFT);
}

/**
* \brief  This API will enable the DMA read/write events of McSPI.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  dmaFlags       Variable used to enable DMA mode for
*                        Rx/Tx events.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'dmaFlags' can take the following values.\n
*         MCSPI_DMA_RX_EVENT  - To enable DMA mode for Rx events.\n
*         MCSPI_DMA_TX_EVENT  - To enable DMA mode for Tx events.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return none.
**/
void McSPIDMAEnable(unsigned int baseAdd, unsigned int dmaFlags,
                    unsigned int chNum)
{
    /* Enable the DMA events. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) |= (dmaFlags & (MCSPI_CH0CONF_DMAR |
                                                          MCSPI_CH0CONF_DMAW));
}

/**
* \brief  This API will disable the DMA read/write events of McSPI.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  dmaFlags       Variable used to disable DMA mode for
*                        Rx/Tx events.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'dmaFlags' can take the following values.\n
*         MCSPI_DMA_RX_EVENT - To disable DMA mode for Rx events.\n
*         MCSPI_DMA_TX_EVENT - To disable DMA mode for Tx events.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return none.
**/
void McSPIDMADisable(unsigned int baseAdd, unsigned int dmaFlags,
                     unsigned int chNum)
{
    /* Disable the DMA events. */
    HWREG(baseAdd + MCSPI_CHCONF(chNum)) &= ~(dmaFlags & (MCSPI_CH0CONF_DMAR |
                                                           MCSPI_CH0CONF_DMAW));
}

/**
* \brief  This API will enable the McSPI Interrupts.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  intFlags       Represents the various interrupts to be enabled.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'intFlags' can take the following values.\n
*         MCSPI_INT_TX_EMPTY(chan) - Transmitter register empty interrupt.\n
*         MCSPI_INT_TX_UNDERFLOW(chan) - Transmitter register underflow
*                                        interrupt.\n
*         MCSPI_INT_RX_FULL(chan) - Receiver register full interrupt.\n
*         MCSPI_INT_RX0_OVERFLOW - Receiver register 0 overflow interrupt.\n
*         MCSPI_INT_EOWKE - End of word count interrupt.\n
*
*         'chan' stands for channel number.
*         Please specify the proper channel number while passing the macros.
*         0 <= chan <= 3 \n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return none.
*
* \note   Please ensure the proper channel number is passed while using the
*         macros.
**/
void McSPIIntEnable(unsigned int baseAdd, unsigned int intFlags)
{
    /* Enable the Interrupts. */
    HWREG(baseAdd + MCSPI_IRQENABLE) |= intFlags;
}

/**
* \brief  This API will disable the McSPI Interrupts.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  intFlags       Represents the various interrupts to be disabled.
*
*         'intFlags' can take the following values.\n
*         MCSPI_INT_TX_EMPTY(chan) - Transmitter register empty interrupt.\n
*         MCSPI_INT_TX_UNDERFLOW(chan) - Transmitter register underflow
*                                        interrupt.\n
*         MCSPI_INT_RX_FULL(chan) - Receiver register full interrupt.\n
*         MCSPI_INT_RX0_OVERFLOW - Receiver register 0 overflow interrupt.\n
*         MCSPI_INT_EOWKE - End of word count interrupt.\n
*
*         'chan' stands for channel number.
*         Please specify the proper channel number while passing the macros.
*         0 <= chan <= 3 \n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return none.
*
* \note   Please ensure the proper channel number is passed while using the
*         macros.
**/
void McSPIIntDisable(unsigned int baseAdd, unsigned int intFlags)
{
    /* Disable the interrupts. */
    HWREG(baseAdd + MCSPI_IRQENABLE) &= ~intFlags;
}

/**
* \brief  This API will set initial delay for first transfer from McSPI
*         peripheral.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  initDelay      Delay used for first transfer.\n
*
*         'initDelay' can take the following values.\n
*         MCSPI_INITDLY_0  - No delay for first transfer.\n
*         MCSPI_INITDLY_4  - Delay of 4 SPI Clock.\n
*         MCSPI_INITDLY_8  - Delay of 8 SPI Clock.\n
*         MCSPI_INITDLY_16 - Delay of 16 SPI Clock.\n
*         MCSPI_INITDLY_32 - Delay of 32 SPI Clock.\n
*
* \return none.
*
* \note:  Please note that this option is available only in single master mode.
**/
void McSPIInitDelayConfig(unsigned int baseAdd, unsigned int initDelay)
{
    /* Clear the INITDLY field of MCSPI_MODULCTRL register. */
    HWREG(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_INITDLY;

    /* Set the INITDLY field with the user sent value. */
    HWREG(baseAdd + MCSPI_MODULCTRL) |= (initDelay & MCSPI_MODULCTRL_INITDLY);
}

/**
* \brief  This API will enable/disable the SYSTEST mode of McSPI peripheral\n.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  sysTest        Value to enable or disable the SYSTEST mode of
*                        operation.\n
*
*         'sysTest' can take the following values.\n
*         MCSPI_SYSTEST_ENABLE  - To enable SYSTEST mode.\n
*         MCSPI_SYSTEST_DISABLE - To disable SYSTEST mode.\n
*
* \return none.
**/
void McSPISysTestModeConfig(unsigned int baseAdd, unsigned int sysTest)
{
    /* Clear the SYSTEM_TEST field of MCSPI_MODULCTRL register. */
    HWREG(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_SYSTEM_TEST;

    /* Set the SYSTEM_TEST field with the user sent value. */
    HWREG(baseAdd + MCSPI_MODULCTRL) |= (sysTest & MCSPI_MODULCTRL_SYSTEM_TEST);
}

/**
* \brief  This API will enable the SPIEN[0:3] for slave signal detection.
*         By using this bit we can accordingly configure the SPIEN pin for
*         slave detection.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  slaveSelect    Will configure the SPIEN[0:3] pin for slave
*                        detection.\n
*
*         'slaveSelect' can take the following values.\n
*         MCSPI_SLAVE_DETECT_n - Detection enabled on SPIEN[n].\n
*
*         For slaveSelect n can range from 0-3.\n
*
* \return none.
*
* \note: This API does not have any significance in master mode.
**/
void McSPISlaveSelect(unsigned int baseAdd, unsigned int slaveSelect)
{
    /* Clear the SPIENSLV field of MCSPI_CHCONF register. */
    HWREG(baseAdd + MCSPI_CHCONF(MCSPI_CH_NUM_0)) &= ~MCSPI_CH0CONF_SPIENSLV;

    /* Set the SPIENSLV field with the user sent value. */
    HWREG(baseAdd + MCSPI_CHCONF(MCSPI_CH_NUM_0)) |= (slaveSelect &
                                                       MCSPI_CH0CONF_SPIENSLV);
}

/**
* \brief  This API will put the data on to the McSPI Channel
*         transmit register.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  txData         32 bit data sent by the user which is put on
*                        to the MCSPI_TX register.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return none.
**/
void McSPITransmitData(unsigned int baseAdd, unsigned int txData,
                       unsigned int chNum)
{
    /* Load the MCSPI_TX register with the data to be transmitted */
    HWREG(baseAdd + MCSPI_TX(chNum)) = txData;
}

/**
* \brief  This API will return the data present in the MCSPI_RX register.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  chNum          Channel number of the McSPI instance used.
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return This API will return the data received in the MCSPI_RX register.
**/
unsigned int McSPIReceiveData(unsigned int baseAdd, unsigned int chNum)
{
    /* Return the data present in the MCSPI_RX register. */
    return(HWREG(baseAdd + MCSPI_RX(chNum)));
}

/**
* \brief  This API will return the status of the McSPI peripheral interrupts.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
*
* \return This API will return the status of the McSPI peripheral interrupts.
*         User can use the following macros to check the status \n
*         MCSPI_INT_TX_EMPTY(chan) - Transmitter register empty for channel n \n
*         MCSPI_INT_TX_UNDERFLOW(chan) - Transmitter register underflow for
*                                        channel n \n
*         MCSPI_INT_RX_FULL(chan) - Receiver register full for channel n \n
*         MCSPI_INT_RX0_OVERFLOW - Receiver register overflow for channel 0 \n
*         MCSPI_INT_EOWKE - End of word count interrupt \n
*         where 0 <= chan <= 3
*
**/
unsigned int McSPIIntStatusGet(unsigned int baseAdd)
{
    /* Return the interrupt status present in the MCSPI_IRQSTATUS register. */
    return(HWREG(baseAdd + MCSPI_IRQSTATUS));
}

/**
* \brief  This API will clear the status of McSPI Interrupts.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  intFlags       Represents the various interrupts to be cleared.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         intFlags can take the following values.\n
*         MCSPI_INT_TX_EMPTY(chan) - Transmitter register empty interrupt.\n
*         MCSPI_INT_TX_UNDERFLOW(chan) - Transmitter register underflow
*                                        interrupt.\n
*         MCSPI_INT_RX_FULL(chan) - Receiver register full interrupt.\n
*         MCSPI_INT_RX0_OVERFLOW - Receiver register 0 overflow interrupt.\n
*         MCSPI_INT_EOW - End of word count interrupt.\n
*
*         'chan' stands for channel number. Please ensure proper channel
*         number is used while passing the macros.
*         0 <= chan <= 3 \n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return none.
*
* \note   Please ensure the proper channel number is used while passing the
*         macros passed.\n
**/
void McSPIIntStatusClear(unsigned int baseAdd, unsigned int intFlags)
{
    /* Clear the SSB bit in the MCSPI_SYST register. */
    HWREG(baseAdd + MCSPI_SYST) &= ~MCSPI_SET_STATUS_BIT;

    /* Clear the interrupt status. */
    HWREG(baseAdd + MCSPI_IRQSTATUS) = intFlags;
}

/**
* \brief  This API will store the value of CLKSPI line into memory location
*         pointed by spiClk when McSPI peripheral is in slave mode.
*
* \param  baseAdd        Memory Address of the McSPI instance used.\n
* \param  spiClk         Will hold the value of spiClk.\n
*
* \return This API will return TRUE if McSPI peripheral is in Slave mode and
*         FALSE if peripheral is in Master mode.
*
* \note   This API will be applicable if McSPI is used in Slave mode only\n.
**/
unsigned int McSPIClkRead(unsigned int baseAdd, unsigned int *spiClk)
{
    unsigned int retVal = FALSE;

    /* Check whether SPIENDIR is used as input. */
    if(HWREG(baseAdd + MCSPI_SYST) & MCSPI_SYST_SPIENDIR)
    {
        retVal = TRUE;
        *spiClk = (HWREG(baseAdd + MCSPI_SYST) & MCSPI_SYST_SPICLK);
    }

    return retVal;
}

/**
* \brief  This API will change the value of McSPI clock to high or low, as
*         programmed by the user when McSPI peripheral is in master mode only\n.
*
* \param  baseAdd        Memory Address of the McSPI instance used\n.
* \param  spiClk         Will hold the value of spiClk \n.
*
*         'spiClk' can take the following values.\n
*
*         MCSPI_CLK_HIGH - Will drive the McSPI clk line high.\n
*         MCSPI_CLK_LOW  - Will drive the McSPI clk line low.\n
*
* \return This API will return TRUE if McSPI peripheral is in Master mode and
*         FALSE if peripheral is in Slave mode.
*
* \note   This API will be applicable if McSPI is used in Master mode.
**/
unsigned int McSPIClkWrite(unsigned int baseAdd, unsigned int spiClk)
{
    unsigned int retVal = FALSE;

    if(!(HWREG(baseAdd + MCSPI_SYST) & MCSPI_SYST_SPIENDIR))
    {
        retVal = TRUE;

        /* Clear the SPICLK field of MCSPI_SYST register. */
        HWREG(baseAdd + MCSPI_SYST) &= ~MCSPI_SYST_SPICLK;

        /* Set the SPICLK field with the user sent value. */
        HWREG(baseAdd + MCSPI_SYST) |= (spiClk & MCSPI_SYST_SPICLK);
    }

    return retVal;
}

/**
* \brief  This API will read the values of SPIDAT_0 or SPIDAT_1 depending on
*         dataDir and provided SPIDAT[0:1] is used only for input.
*
* \param  baseAdd        Memory Address of the McSPI instance used.\n
* \param  dataPinDir     Direction of SPIDAT[1:0] pins.\n
* \param  spiDat         Memory location at which data pin data has to be
*                        placed.
*
*         'dataPinDir' can take the following values.\n
*         MCSPI_DAT1_IN - SPIDAT_1 is used as input pin.\n
*         MCSPI_DAT0_IN - SPIDAT_0 is used as input pin.\n
*
* \return This API will return TRUE if SPIDAT[1:0] pins are used as input
*         else FALSE.
*
* \note   This API will be applicable if SPIDAT[1:0] pins are used only
*         as input.
**/
unsigned int McSPIDataPinRead(unsigned int baseAdd, unsigned int dataPinDir,
                              unsigned int *spiDat)
{
    unsigned int retVal = FALSE;

    if(dataPinDir == (HWREG(baseAdd + MCSPI_SYST) & MCSPI_SYST_SPIDATDIR1))
    {
        retVal = TRUE;

        /* Read the SPIDAT_1 value. */
        *spiDat = (HWREG(baseAdd + MCSPI_SYST) & MCSPI_SYST_SPIDAT_1);
    }

    else if(dataPinDir == (HWREG(baseAdd + MCSPI_SYST) & MCSPI_SYST_SPIDATDIR0))
    {
        retVal = TRUE;

        /* Read the SPIDAT_0 value. */
        *spiDat = (HWREG(baseAdd + MCSPI_SYST) & MCSPI_SYST_SPIDAT_0);
    }

    else
    {

    }

    return retVal;
}

/**
* \brief  This API will write spiDat into SPIDAT_0 or SPIDAT_1 field of
*         MCSPI_SYST register depending on the dataDir values.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  dataPinDir     Direction of SPIDAT[1:0] pins.
* \param  spiDat         Data which has to be written to SPIDAT field.\n
*
*         'dataPinDir' can take the following values.\n
*         MCSPI_DAT1_OUT - SPIDAT_1 is used as output pin.\n
*         MCSPI_DAT0_OUT - SPIDAT_0 is used as output pin.\n
*
*         'spiDat' can take the following values.\n
*         MCSPI_DATA_PIN_HIGH - Will drive the data pin high.\n
*         MCSPI_DATA_PIN_LOW - Will drive the data pin low.\n
*
* \return This API will return TRUE if SPIDAT[1:0] pins are used as output
*         else FALSE.
*
* \note   This API will be applicable if SPIDAT[1:0] pins are used only
*         as output.
**/
unsigned int McSPIDataPinWrite(unsigned int baseAdd, unsigned int dataPinDir,
                               unsigned int spiDat)
{
    unsigned int retVal = FALSE;

    if(dataPinDir == (HWREG(baseAdd + MCSPI_SYST) & MCSPI_SYST_SPIDATDIR1))
    {
        retVal = TRUE;

        /* Clear the SPIDAT_1 field of MCSPI_SYST register. */
        HWREG(baseAdd + MCSPI_SYST) &= ~MCSPI_SYST_SPIDAT_1;

        /* Set the SPIDAT_1 field with the user sent value. */
        HWREG(baseAdd + MCSPI_SYST) |= (spiDat << MCSPI_SYST_SPIDAT_1_SHIFT);
    }

    else if(dataPinDir == (HWREG(baseAdd + MCSPI_SYST) & MCSPI_SYST_SPIDATDIR0))
    {
        retVal = TRUE;

        /* Clear the SPIDAT_0 field of MCSPI_SYST register. */
        HWREG(baseAdd + MCSPI_SYST) &= ~MCSPI_SYST_SPIDAT_0;

        /* Set the SPIDAT_0 field with the user sent value. */
        HWREG(baseAdd + MCSPI_SYST) |= (spiDat << MCSPI_SYST_SPIDAT_0_SHIFT);
    }

    else
    {

    }

    return retVal;
}

/**
* \brief  This API will read the data present on the SPIEN line.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  spiEn          Memory location at which read value is stored.
* \param  spiEnNum       The instance of SPIEN used.\n
*
*         'spiEnNum' can take the following values.\n
*         MCSPI_SPIEN_n - SPIEN_n is used.\n
*
*         For spiEnNum n can range from 0-3.\n
*
* \return This API will return TRUE if direction of SPIEN is used as input
*         else FALSE.
*
* \note   This API will be applicable if SPIENDIR is configured as input.
*
**/
unsigned int McSPICSRead(unsigned int baseAdd, unsigned int *spiEn,
                         unsigned int spiEnNum)
{
    unsigned int retVal = FALSE;

    if((HWREG(baseAdd + MCSPI_SYST) & MCSPI_SYST_SPIENDIR))
    {
        retVal = TRUE;

        /* Read the SPIEN value. */
        *spiEn = (HWREG(baseAdd + MCSPI_SYST) & spiEnNum);
    }

    return retVal;
}

/**
* \brief  This API will change the value to high or low of SPIEN_n pin where
*         n can be 0-3 when SPIENDIR is configured as output.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  spiEnLvl       Value to be written into SPIEN_n field where
*                        n is 0-3.
* \param  spiEnNum       The instance of SPIEN used.\n
*
*         'spiEnLvl' can take the following values.\n
*
*         MCSPI_SPIEN_HIGH - SPIEN is driven high.\n
*         MCSPI_SPIEN_LOW - SPIEN is driven low.\n
*
*         'spiEnNum' can take the following values.\n
*         MCSPI_SPIEN_n - SPIEN_n is used.\n
*
*         For spiEnNum n can range from 0-3.\n
*
* \return This API will return TRUE if direction of SPIEN is used as output
*         else FALSE.
*
* \note   This API will be applicable if SPIENDIR is configured as output.
*
**/
unsigned int McSPICSWrite(unsigned int baseAdd, unsigned int spiEnLvl,
                          unsigned int spiEnNum)
{
    unsigned int retVal = FALSE;

    if(!(HWREG(baseAdd + MCSPI_SYST) & MCSPI_SYST_SPIENDIR))
    {
        retVal = TRUE;

        if(spiEnNum == MCSPI_SPIEN_0)
        {
            /* Clear the SPIEN_0 field of MCSPI_SYST register. */
            HWREG(baseAdd + MCSPI_SYST) &= ~MCSPI_SYST_SPIEN_0;

            /* Set the SPIEN_0 field with the user sent value. */
            HWREG(baseAdd + MCSPI_SYST) |= (spiEnLvl & MCSPI_SYST_SPIEN_0);
        }

        else if(spiEnNum == MCSPI_SPIEN_1)
        {
            /* Clear the SPIEN_1 field of MCSPI_SYST register. */
            HWREG(baseAdd + MCSPI_SYST) &= ~MCSPI_SYST_SPIEN_1;

            /* Set the SPIEN_1 field with the user sent value. */
            HWREG(baseAdd + MCSPI_SYST) |= ((spiEnLvl <<
                                             MCSPI_SYST_SPIEN_1_SHIFT) &
                                             MCSPI_SYST_SPIEN_1);
        }

        else if(spiEnNum == MCSPI_SPIEN_2)
        {
            /* Clear the SPIEN_2 field of MCSPI_SYST register. */
            HWREG(baseAdd + MCSPI_SYST) &= ~MCSPI_SYST_SPIEN_2;

            /* Set the SPIEN_2 field with the user sent value. */
            HWREG(baseAdd + MCSPI_SYST) |= ((spiEnLvl <<
                                             MCSPI_SYST_SPIEN_2_SHIFT) &
                                             MCSPI_SYST_SPIEN_2);
        }

        else if(spiEnNum == MCSPI_SPIEN_3)
        {
            /* Clear the SPIEN_3 field of MCSPI_SYST register. */
            HWREG(baseAdd + MCSPI_SYST) &= ~MCSPI_SYST_SPIEN_3;

            /* Set the SPIEN_3 field with the user sent value. */
            HWREG(baseAdd + MCSPI_SYST) |= ((spiEnLvl <<
                                             MCSPI_SYST_SPIEN_3_SHIFT) &
                                             MCSPI_SYST_SPIEN_3);
        }

        else
        {

        }
    }

    return retVal;
}

/**
* \brief  This API will return the status of the McSPI channel currently in
*         use.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  chNum          Channel used for communication.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
* \return This API will return the status of the McSPI channel status
*         register.
*         User can use the following macros to check the status \n
*         MCSPI_CH_STAT_RXS_FULL - Receiver register is full \n
*         MCSPI_CH_STAT_TXS_EMPTY - Transmitter register is full \n
*         MCSPI_CH_STAT_EOT - End of transfer status \n
*         MCSPI_CH_TXFFE - FIFO transmit buffer empty status \n
*         MCSPI_CH_TXFFF - FIFO transmit buffer full status \n
*         MCSPI_CH_RXFFE - FIFO receive buffer empty status \n
*         MCSPI_CH_RXFFF - FIFO receive buffer full status \n
**/
unsigned int McSPIChannelStatusGet(unsigned int baseAdd, unsigned int chNum)
{
    /* Return the status from MCSPI_CHSTAT register. */
    return(HWREG(baseAdd + MCSPI_CHSTAT(chNum)));
}

/**
* \brief  This API will enable/disable multiple word OCP access for McSPI
*         peripheral.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  moa            Used to enable/disable MOA in McSPI peripheral.\n
*
*         'moa' can take the following values.\n
*         MCSPI_MOA_ENABLE  - Enable MOA.\n
*         MCSPI_MOA_DISABLE - Disable MOA.\n
*
* \return none.
**/
void McSPIMultipleWordAccessConfig(unsigned int baseAdd, unsigned int moa)
{
    /* Clear the MOA field of MCSPI_MODULCTRL register. */
    HWREG(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_MOA;

    /* Set the MOA fields with the user sent value. */
    HWREG(baseAdd + MCSPI_MODULCTRL) |= (moa & MCSPI_MODULCTRL_MOA);
}

/**
* \brief  This API will enable/disable the FIFO DMA address 256-bit aligned
*         feature of McSPI peripheral.
*
* \param  baseAdd       Memory Address of the McSPI instance used.
* \param  fdaa          Used to enable/disable FDAA in McSPI peripheral.\n
*
*        'fdaa' can take the following values.\n
*        MCSPI_FDAA_ENABLE  - Enable FDAA.\n
*        MCSPI_FDAA_DISABLE - Disable FDAA.\n
*
* \return none.
**/
void McSPIFIFODatManagementConfig(unsigned int baseAdd, unsigned int fdaa)
{
    /* Clear the FDAA field of MCSPI_MODULCTRL register. */
    HWREG(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_FDAA;

    /* Set the FDAA field with the user sent value. */
    HWREG(baseAdd + MCSPI_MODULCTRL) |= (fdaa & MCSPI_MODULCTRL_FDAA);
}

/**
* \brief  This API will enable/disable the McSPI peripheral in autoIdle mode.
*
* \param  baseAdd       Memory Address of the McSPI instance used.\n
* \param  autoIdle      Used to enable/disable AUTOIDLE mode in McSPI
*                       peripheral.\n
*
*         'autoIdle' can take the following values.\n
*         MCSPI_AUTO_IDLE_ENABLE  - Enable AUTOIDLE.\n
*         MCSPI_AUTO_IDLE_DISABLE - Disable AUTOIDLE.\n
*
* \return none.
**/
void McSPIAutoIdleConfig(unsigned int baseAdd, unsigned int autoIdle)
{
    /* Clear the AUTOIDLE field of MCSPI_SYSCONFIG register. */
    HWREG(baseAdd + MCSPI_SYSCONFIG) &= ~MCSPI_SYSCONFIG_AUTOIDLE;

    /* Set the AUTOIDLE field with the user sent value. */
    HWREG(baseAdd + MCSPI_SYSCONFIG) |= (autoIdle & MCSPI_SYSCONFIG_AUTOIDLE);
}

/**
* \brief  This API will enable/disable multiple word access for McSPI
*         peripheral.
*
* \param  baseAdd       Memory Address of the McSPI instance used.
* \param  smartIdle     Used to set modes for smart idle mode.\n

*         'smartIdle' can take the following values.\n
*         MCSPI_SIDLE_MODE_FORCE - Inactive or Force mode.\n
*         MCSPI_SIDLE_MODE_NOIDLE - No Idle mode.\n
*         MCSPI_SIDLE_MODE_SMART - Smart Idle mode.\n
*
* \return none.
**/
void McSPISmartIdleConfig(unsigned int baseAdd, unsigned int smartIdle)
{
    /* Clear the SIDLE field of MCSPI_SYSCONFIG register. */
    HWREG(baseAdd + MCSPI_SYSCONFIG) &= ~MCSPI_SYSCONFIG_SIDLEMODE;

    /* Set the SIDLE field with the user sent values. */
    HWREG(baseAdd + MCSPI_SYSCONFIG) |= (smartIdle & MCSPI_SYSCONFIG_SIDLEMODE);
}

/**
* \brief  This API will set the clock activity for McSPI peripheral.
*
* \param  baseAdd      Memory Address of the McSPI instance used.
* \param  clkAct       Clock activity used.\n
*
*         'clkAct' can take the following values.\n
*         MCSPI_CLK_ACTIVITY_NONE - OCP and functional clocks not used.\n
*         MCSPI_CLK_ACTIVITY_OCP  - OCP clock only may be used.\n
*         MCSPI_CLK_ACTIVITY_FUNC - Functional clock only may be used.\n
*         MCSPI_CLK_ACTIVITY_BOTH - OCP and Functional clocks are used.\n
*
* \return none.
**/
void McSPIClkActivityConfig(unsigned int baseAdd, unsigned int clkAct)
{
    /* Clear the CLOCKACTIVITY field of MCSPI_SYSCONFIG register. */
    HWREG(baseAdd + MCSPI_SYSCONFIG) &= ~MCSPI_SYSCONFIG_CLOCKACTIVITY;

    /* Set the CLOCKACTIVITY field with the user sent value. */
    HWREG(baseAdd + MCSPI_SYSCONFIG) |= (clkAct & MCSPI_SYSCONFIG_CLOCKACTIVITY);
}

/***************************** END OF FILE ************************************/
