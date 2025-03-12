/**
*  \file spi.c
*    
*  \brief SPI device abstraction layer APIs
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

/* HW Macros */
#include "hw_types.h"

/* Driver APIs */
#include "hw_spi.h"

/**
* \brief - It will configure the prescalar to generate required spi clock\n.
*
* \param - baseAdd     is Memory Address of the SPI instance used\n.
* \param - moduleClk   is the input clk to SPI module from PLL\n.
* \param - spiClk      is the spi bus speed\n.
* \param - dataFormat  is instance of the data format register used\n.
*
* \return none.
**/
void SPIClkConfigure(unsigned int baseAdd, unsigned int moduleClk,
                     unsigned int spiClk, unsigned int dataFormat)
{
    unsigned int prescale;

    prescale = (moduleClk/spiClk) - 1;

    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) =  \
        (SPI_SPIFMT_PRESCALE & (prescale << SPI_SPIFMT_PRESCALE_SHIFT));
}
/**
* \brief - It will Enable SPI module\n.
*
* \param - baseAdd  is the Memory Address of the SPI instance used\n.
*    
* \return none.
**/    
void SPIEnable(unsigned int baseAdd)
{
    HWREG(baseAdd + SPI_SPIGCR1) |= SPI_SPIGCR1_ENABLE;           
}
/**
* \brief - It will put SPI in to reset state.\n
*
* \param - baseAdd is the Memory Address of the SPI instance used.\n
*
* \return none.
**/
void SPIReset(unsigned int baseAdd)
{
    HWREG(baseAdd + SPI_SPIGCR0) = ~(SPI_SPIGCR0_RESET);
}
/**
* \brief - Brings SPI out of reset state.
*
* \param - baseAdd is the Memory Address of the SPI instance used.\n
*
* \return none.
**/
void SPIOutOfReset(unsigned int baseAdd)
{
    HWREG(baseAdd + SPI_SPIGCR0) =  SPI_SPIGCR0_RESET; 
}      
/**
* \brief - Configures SPI to master or slave mode.
*
* \param - baseAdd is the Memory Address of the SPI instance used.\n
*
* \return none.
**/
void SPIModeConfigure(unsigned int baseAdd, unsigned int flag)
{
    HWREG(baseAdd + SPI_SPIGCR1) |= flag;
}

/**
* \brief - Configures SPI Pin Control Registers.\n
*
* \param - baseAdd is the Memory Address of the SPI instance used.\n
* \param - idx     is the Pin Control register number.It can take
*                  any integer value between 0 and 5.\n
*
* \param - flag    is to indicate to whether to (1)read from Pin Control
*                  Register or to (0)write to Pin Control Register.\n
*
* \param - val     is a value/return argument which has the value to be written
*                  in case of writes or the value read in case of reads
*
* \return none.\n
**/
void SPIPinControl(unsigned int baseAdd, unsigned int idx,
                           unsigned int flag, unsigned int *val)
{
    if (0 == flag)
    {
        HWREG(baseAdd + SPI_SPIPC(idx)) = *val;
    }
    else
    {
        *val = HWREG(baseAdd + SPI_SPIPC(idx));
    }
}
/**
* \brief - Configures SPI CS and ENA Delay in SPIDELAY Register.\n
*
* \param - baseAdd  is Memory Address of the SPI instance.\n
* \param - c2edelay is the Chip-select-active-to-SPIx_ENA-signal
*                            -active-time-out.\n
*
* \param - t2edelay is the Transmit-data-finished-to-SPIx_ENA-pin
*                            -inactive-time-out.\n
*
* \param - t2cdelay is the Transmit-end-to-chip-select-inactive-delay.\n
* \param - c2tdelay is the Chip-select-active-to-transmit-start-delay.\n
*
* \return none.
*
* Note: SPIx_CS and SPI_ENA are active low pins.
**/
void SPIDelayConfigure(unsigned int baseAdd, unsigned int c2edelay,
                       unsigned int t2edelay, unsigned int t2cdelay,
                       unsigned int c2tdelay)
{
    HWREG(baseAdd + SPI_SPIDELAY) |= (c2edelay & SPI_SPIDELAY_C2EDELAY);

    HWREG(baseAdd + SPI_SPIDELAY) |= (SPI_SPIDELAY_T2EDELAY & 
                                     (t2edelay << SPI_SPIDELAY_T2EDELAY_SHIFT));

    HWREG(baseAdd + SPI_SPIDELAY) |= (SPI_SPIDELAY_T2CDELAY &
                                     (t2cdelay << SPI_SPIDELAY_T2CDELAY_SHIFT));

    HWREG(baseAdd + SPI_SPIDELAY) |= (SPI_SPIDELAY_C2TDELAY &
                                     (c2tdelay << SPI_SPIDELAY_C2TDELAY_SHIFT));
}  
/**
* \brief - Sets the default value for CS pin(line)  when no transmission is
*            is performed by writing to SPIDEF Reg.\n
*
* \param - baseAdd is the Memory Address of the SPI instance used.\n
* \param - dcsval  is the value written to SPIDEF register to set default
*          value on CS pin(line).\n
*
* \return none.
**/
void SPIDefaultCSSet(unsigned int baseAdd, unsigned char dcsval)
{
    HWREG(baseAdd + SPI_SPIDEF) = dcsval;
}
/**
* \brief - Configures the SPIDat1 Register.It won't write to TX part
*            of the SPIDat1 Register.\n
*
* \param - baseAdd  is the Memory Address of the SPI data instance used.\n
* \param - flag is value to Configure CSHOL,Wait Delay Conter Enable bit  
*          and to select the appropriate DataFormat register.\n
*
*         flag can take following values.\n
*               
*         SPI_CSHOLD            - To Hold the CS line active after data Transfer
*                                 untill new data and Control information is
*                                 loaded.\n
*             
*         SPI_DELAY_COUNTER_ENA - Enables Delay Counter.\n
*
*         SPI_DATA_FORMAT0      - To select DataFormat Register 0.\n            
*         SPI_DATA_FORMAT1      - To select DataFormat Register 1.\n            
*         SPI_DATA_FORMAT2      - To select DataFormat Register 2.\n            
*         SPI_DATA_FORMAT3      - To select DataFormat Register 3.\n
*
* \param - cs is the value to driven on CS pin(line).\n
*
* \return none.
**/
void SPIDat1Config(unsigned int baseAdd, unsigned int flag, unsigned char cs)
{
    unsigned char *ptr = (unsigned char*)(baseAdd + SPI_SPIDAT1);
    unsigned char dcs;

    *(ptr+3) = (char)((flag >> 24) | (flag & (SPI_SPIDAT1_DFSEL >> 
                                      SPI_SPIDAT1_DFSEL_SHIFT)));

    dcs = HWREG(baseAdd + SPI_SPIDEF ) & (SPI_SPIDEF_CSDEF);          

    *(ptr+2) = cs ^ dcs;
}
/**
* \brief - Trasmits Data from TX part of SPIDAT1 register.\n
*
* \param - baseAdd is the Memory address of the SPI instance used.\n
* \param - data    is the data transmitted out of SPI.\n
*
* \return none.
**/
void SPITransmitData1(unsigned int baseAdd, unsigned int data)
{
    HWREG(baseAdd + SPI_SPIDAT1) = \
        (HWREG(baseAdd + SPI_SPIDAT1) & ~ SPI_SPIDAT1_TXDATA) | data;
}
/**
* \brief - It Disables Transmit-end-to-chip-select-inactive-delay(t2cdelay)
*          and Chip-select-active-to-transmit-start-delay(c2tdelay).\n
*
* \param - baseAdd is the Memory address of the the SPI instance used.\n
* \param - dataFormat is the value to select the Format register.\n
*
*            dataFormat can take following value.\n
*         
*            SPI_DATA_FORMAT0      - To select DataFormat Register 0.\n
*            SPI_DATA_FORMAT1      - To select DataFormat Register 1.\n
*            SPI_DATA_FORMAT2      - To select DataFormat Register 2.\n
*            SPI_DATA_FORMAT3      - To select DataFormat Register 3.\n
*
* \return none.
**/            
void SPICSTimerDisable(unsigned int baseAdd, unsigned int dataFormat)
{
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) |= SPI_SPIFMT_DISCSTIMERS;
}
/**
* \brief - It Enables Transmit-end-to-chip-select-inactive-delay(t2cdelay) and
*          Chip-select-active-to-transmit-start-delay(c2tdelay).\n
*
* \param - baseAdd is the Memory address of the the SPI instance used.\n
* \param - dataFormat is the value to select the Format register.\n
*
*            dataFormat can take following value.\n
*
*            SPI_DATA_FORMAT0      - To select DataFormat Register 0.\n
*            SPI_DATA_FORMAT1      - To select DataFormat Register 1.\n
*            SPI_DATA_FORMAT2      - To select DataFormat Register 2.\n
*            SPI_DATA_FORMAT3      - To select DataFormat Register 3.\n
*
* \return none.
**/

void SPICSTimerEnable(unsigned int baseAdd, unsigned int dataFormat)
{
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) &=  ~SPI_SPIFMT_DISCSTIMERS;
}
/**
* \brief - It Set the Charcter length.\n
*
* \param - baseAdd is the Memory address of the the SPI instance used.\n
* \param - flag    is the value which determines the number of the
*          charcters to be transmitted .\n
*
* \param - dataFormat is the value to select the Format register.\n
*
*            dataFormat can take following value.\n
*
*            SPI_DATA_FORMAT0      - To select DataFormat Register 0.\n
*            SPI_DATA_FORMAT1      - To select DataFormat Register 1.\n
*            SPI_DATA_FORMAT2      - To select DataFormat Register 2.\n
*            SPI_DATA_FORMAT3      - To select DataFormat Register 3.\n
*
* \return none.
**/
void SPICharLengthSet(unsigned int baseAdd, unsigned int numOfChar,
                      unsigned int dataFormat)
{
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) &= ~SPI_SPIFMT_CHARLEN;
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) |= numOfChar;
}
/**
* \brief - It Configures SPI Clock's Phase and Polarity.
*
* \param - baseAdd is the Memory address of the the SPI instance used.\n
* \param - flag    is the value which determines Phase and Polarity
*          of the Clock..\n
*
*            flag can take following values.\n
*
*            SPI_CLK_POL_HIGH    - Clock is High Before and after data transfer.
*            SPI_CLK_POL_LOW     - Clock is Low  Before and after data transfer.
*            SPI_CLK_INPHASE     - Clock is not Delayed.
*            SPI_CLK_OUTOFPHASE  - Clock is Delayed by half clock cycle.
*
* \param - dataFormat is the value to select the Format register.\n
*
*            dataFormat can take following value.\n
*
*            SPI_DATA_FORMAT0      - To select DataFormat Register 0.\n
*            SPI_DATA_FORMAT1      - To select DataFormat Register 1.\n
*            SPI_DATA_FORMAT2      - To select DataFormat Register 2.\n
*            SPI_DATA_FORMAT3      - To select DataFormat Register 3.\n
*
* \return none.
**/
void SPIConfigClkFormat(unsigned int baseAdd, unsigned int flag,
                        unsigned int dataFormat)
{
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) |= flag;
}
/**
* \brief - It Configures SPI to Transmit MSB bit first during Data transfer.
*
* \param - baseAdd is the Memory address of the the SPI instance used.\n
* \param - dataFormat is the value to select the Format register.\n
*
*            dataFormat can take following value.\n
*
*            SPI_DATA_FORMAT0      - To select DataFormat Register 0.\n
*            SPI_DATA_FORMAT1      - To select DataFormat Register 1.\n
*            SPI_DATA_FORMAT2      - To select DataFormat Register 2.\n
*            SPI_DATA_FORMAT3      - To select DataFormat Register 3.\n
*
* \return none.
**/

void SPIShiftMsbFirst(unsigned int baseAdd, unsigned int dataFormat)
{
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) &= ~SPI_SPIFMT_SHIFTDIR;
}
/**
* \brief - It Configures SPI to Transmit LSB bit first during Data transfer.
*
* \param - baseAdd is the Memory address of the the SPI instance used.\n
* \param - dataFormat is the value to select the Format register.\n
*
*            dataFormat can take following value.\n
*
*            SPI_DATA_FORMAT0      - To select DataFormat Register 0.\n
*            SPI_DATA_FORMAT1      - To select DataFormat Register 1.\n
*            SPI_DATA_FORMAT2      - To select DataFormat Register 2.\n
*            SPI_DATA_FORMAT3      - To select DataFormat Register 3.\n
*
* \return none.
**/
void SPIShiftLsbFirst(unsigned int baseAdd, unsigned int dataFormat)
{
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) |= SPI_SPIFMT_SHIFTDIR;
}
/**
* \brief - It Enables Parity in SPI and also configures Even or Odd Parity.\n
*
* \param - baseAdd is the Memory address of the the SPI instance used.\n
* \param - flag    is the value determines whether odd or even Parity.\n
*
*            flag can take following values.\n
*          
*            SPI_ODD_PARITY   -  selects odd parity
*            SPI_EVEN_PARITY  -  selects even parity
*
* \param - dataFormat is the value to select the Format register.\n
*
*            dataFormat can take following value.\n
*
*            SPI_DATA_FORMAT0      - To select DataFormat Register 0.\n
*            SPI_DATA_FORMAT1      - To select DataFormat Register 1.\n
*            SPI_DATA_FORMAT2      - To select DataFormat Register 2.\n
*            SPI_DATA_FORMAT3      - To select DataFormat Register 3.\n
*
* \return none.
**/
void SPIParityEnable(unsigned int baseAdd, unsigned int flag,
                     unsigned int dataFormat)
{
    /* First set the required parity */
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) = \
        (HWREG(baseAdd + SPI_SPIFMT(dataFormat)) & ~ SPI_SPIFMT_PARPOL) | flag;

    /* Enable parity */
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) |= SPI_SPIFMT_PARENA;
}
/**
* \brief - It Disables Parity in SPI.\n
*
* \param - baseAdd is the Memory address of the the SPI instance used.\n
* \param - dataFormat is the value to select the Format register.\n
*
*            dataFormat can take following value.\n
*
*            SPI_DATA_FORMAT0      - To select DataFormat Register 0.\n
*            SPI_DATA_FORMAT1      - To select DataFormat Register 1.\n
*            SPI_DATA_FORMAT2      - To select DataFormat Register 2.\n
*            SPI_DATA_FORMAT3      - To select DataFormat Register 3.\n
*
* \return none.
**/
void SPIParityDisable(unsigned int baseAdd, unsigned int dataFormat)
{
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) &= ~SPI_SPIFMT_PARENA;
}
/**
* \brief - It sets the Delay between SPI transmission.\n
*
* \param - baseAdd is the Memory address of the the SPI instance used.\n
* \param - flag    is the value determines amount of delay.\n
* \param - dataFormat is the value to select the Format register.\n
*
*            dataFormat can take following value.\n
*
*            SPI_DATA_FORMAT0      - To select DataFormat Register 0.\n
*            SPI_DATA_FORMAT1      - To select DataFormat Register 1.\n
*            SPI_DATA_FORMAT2      - To select DataFormat Register 2.\n
*            SPI_DATA_FORMAT3      - To select DataFormat Register 3.\n
*
* \return none.
**/
void SPIWdelaySet(unsigned int baseAdd, unsigned int flag,
                  unsigned int dataFormat)
{
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) |= flag;
}
/**
* \brief - It Configures SPI Master to wait for SPIx_ENA signal.\n
*
* \param - baseAdd is the Memory address of the the SPI instance used.\n
* \param - dataFormat is the value to select the Format register.\n
*
*            dataFormat can take following value.\n
*
*            SPI_DATA_FORMAT0      - To select DataFormat Register 0.\n
*            SPI_DATA_FORMAT1      - To select DataFormat Register 1.\n
*            SPI_DATA_FORMAT2      - To select DataFormat Register 2.\n
*            SPI_DATA_FORMAT3      - To select DataFormat Register 3.\n
*
* \return none.
*
* Note:It is applicable only in SPI Master Mode.SPIx_ENA is a active 
*      low signal
**/
void SPIWaitEnable(unsigned int baseAdd, unsigned int dataFormat)
{
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) |= SPI_SPIFMT_WAITENA;
}
/**
* \brief - It Configures SPI Master not to wait for SPIx_ENA signal.\n
*
* \param - baseAdd is the Memory address of the the SPI instance used.\n
* \param - dataFormat is the value to select the Format register.\n
*
*            dataFormat can take following value.\n
*
*            SPI_DATA_FORMAT0      - To select DataFormat Register 0.\n
*            SPI_DATA_FORMAT1      - To select DataFormat Register 1.\n
*            SPI_DATA_FORMAT2      - To select DataFormat Register 2.\n
*            SPI_DATA_FORMAT3      - To select DataFormat Register 3.\n
*
* \return none.
*
* Note:It is applicable only in SPI Master Mode.SPIx_ENA is a active
*      low signal
**/
void SPIWaitDisable(unsigned int baseAdd, unsigned int dataFormat)
{
    HWREG(baseAdd + SPI_SPIFMT(dataFormat)) &= ~SPI_SPIFMT_WAITENA;
}
/**
* \brief - It Configures SPI to Map interrupts to interrupt line INT1.\n
*
* \param - baseAdd is the Memory Address of the SPI instance used.\n
* \param - flag    is the interrupts required to be mapped.\n
*
*            flag can take following values.\n
*
*            SPI_DATALEN_ERR_INTLVL     - Data length error interrupt level.\n
*            SPI_TIMEOUT_INTLVL         - TimeOut length error interrupt level.\n
*            SPI_PARITY_ERR_INTLVL      - Parity error interrupt level.\n
*            SPI_DESYNC_SLAVE_INTLVL    - Desyncrozied slave interrupt level.\n
*            SPI_BIT_ERR_INTLVL         - Bit error interrupt level.\n
*            SPI_RECV_OVERRUN_INTLVL    - Receive Overrun interrupt level.\n
*            SPI_RECV_INTLVL            - Receive interrupt level.\n
*            SPI_TRANSMIT_INTLVL        - Transmit interrupt level.\n
*
* \return none.
*
**/    
void SPIIntLevelSet(unsigned int baseAdd, unsigned int flag)
{
    HWREG(baseAdd + SPI_SPILVL) |= flag;
}
/**
* \brief - It Enables the interrupts.
*
* \param - baseAdd is the Memory Address of the SPI instance used.\n
* \param - flag    is the interrupts required to be Enabled.\n
*
*            flag can take following values.\n
*
*            SPI_DATALEN_ERR_INT     - Data length error interrupt.\n
*            SPI_TIMEOUT_INT         - TimeOut length error interrupt.\n
*            SPI_PARITY_ERR_INT      - Parity error interrupt.\n
*            SPI_DESYNC_SLAVE_INT    - Desyncrozied slave interrupt.\n
*            SPI_BIT_ERR_INT         - Bit error interrupt.\n
*            SPI_RECV_OVERRUN_INT    - Receive Overrun interrupt.\n
*            SPI_RECV_INT            - Receive interrupt.\n
*            SPI_TRANSMIT_INT        - Transmit interrupt.\n
*
* \return none.
*
**/
void SPIIntEnable(unsigned int baseAdd, unsigned int flag)
{
    HWREG(baseAdd + SPI_SPIINT0) |= flag;
}
/**
* \brief - It Disables the interrupts.
*
* \param - baseAdd is the Memory Address of the SPI instance used.\n
* \param - flag    is the interrupts required to be Enabled.\n
*
*            flag can take following values.\n
*
*            SPI_DATALEN_ERR_INT     - Data length error interrupt.\n
*            SPI_TIMEOUT_INT         - TimeOut length error interrupt.\n
*            SPI_PARITY_ERR_INT      - Parity error interrupt.\n
*            SPI_DESYNC_SLAVE_INT    - Desyncrozied slave interrupt.\n
*            SPI_BIT_ERR_INT         - Bit error interrupt.\n
*            SPI_RECV_OVERRUN_INT    - Receive Overrun interrupt.\n
*            SPI_RECV_INT            - Receive interrupt.\n
*            SPI_TRANSMIT_INT        - Transmit interrupt.\n
*
* \return none.
*
**/
void SPIIntDisable(unsigned int baseAdd, unsigned int flag)
{
    HWREG(baseAdd + SPI_SPIINT0) &= ~(flag);
}
/**
* \brief - It clears Status of interrupts.
*
* \param - baseAdd is the Memory Address of the SPI instance used.\n
* \param - flag    is the interrupts whose status needs to be cleared.\n
*
*            flag can take following values.\n
*
*            SPI_DATALEN_ERR_INT     - Data length error interrupt.\n
*            SPI_TIMEOUT_INT         - TimeOut length error interrupt.\n
*            SPI_PARITY_ERR_INT      - Parity error interrupt.\n
*            SPI_DESYNC_SLAVE_INT    - Desyncrozied slave interrupt.\n
*            SPI_BIT_ERR_INT         - Bit error interrupt.\n
*            SPI_RECV_OVERRUN_INT    - Receive Overrun interrupt.\n
*            SPI_RECV_INT            - Receive interrupt.\n
*            SPI_TRANSMIT_INT        - Transmit interrupt.\n
*            SPI_DMA_REQUEST_ENA_INT - DMA request interrupt.\n
*
* \return none.
*
**/
void SPIIntStatusClear(unsigned int baseAdd, unsigned int flag)
{
    HWREG(baseAdd + SPI_SPIFLG) = flag;
}
/**
* \brief - It reads the  Status of interrupts.
*
* \param - baseAdd is the Memory Address of the SPI instance used.\n
* \param - flag    is the interrupts whose status needs to be read.\n
*
*            flag can take following values.\n
*
*            SPI_DATALEN_ERR_INT     - Data length error interrupt.\n
*            SPI_TIMEOUT_INT         - TimeOut length error interrupt.\n
*            SPI_PARITY_ERR_INT      - Parity error interrupt.\n
*            SPI_DESYNC_SLAVE_INT    - Desyncrozied slave interrupt.\n
*            SPI_BIT_ERR_INT         - Bit error interrupt.\n
*            SPI_RECV_OVERRUN_INT    - Receive Overrun interrupt.\n
*            SPI_RECV_INT            - Receive interrupt.\n
*            SPI_TRANSMIT_INT        - Transmit interrupt.\n
*            SPI_DMA_REQUEST_ENA_INT - DMA request interrupt.\n
*
* \returns status of interrupt.
*
**/
unsigned int SPIIntStatus(unsigned int baseAdd, unsigned int flag)
{
    unsigned int status;

    status = ( HWREG(baseAdd + SPI_SPIFLG) & flag );

    return(status);
}
/**
* \brief - It recevies data by reading from SPIBUF register.\n
*
* \param - baseAdd is the memory instance to be used.\n
*
* \returns received data.
**/
unsigned int SPIDataReceive(unsigned int baseAdd)
{
    return (HWREG(baseAdd + SPI_SPIBUF) &  SPI_SPIBUF_RXDATA);
}
/**
* \brief - It returns the vector of the pending interrupt at interrupt line INT1.\n
*
* \param - baseAdd is the memory instance to be used.\n
*
* \returns vector of the pending interrupt at interrupt line INT1.
**/  
unsigned int  SPIInterruptVectorGet(unsigned int baseAdd)
{
   unsigned int intVectorCode; 
   intVectorCode = HWREG(baseAdd + SPI_INTVEC1);

   return (intVectorCode >> 1);
}
