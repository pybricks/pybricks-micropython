/**
 * \file  i2c.c
 *    
 * \brief I2C device abstraction layer APIs
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

/* HW Macros and Peripheral Defines */
#include "hw_i2c.h"
#include "i2c.h"
#include "hw_types.h"

/*******************************************************************************
*                       INTERNAL MACRO DEFINITIONS
*******************************************************************************/
#define   I2C_TX_RX_EVENT_DISABLE    0x00

/**
* \brief  This function initializes operation of the I2C Master block.  Upon
*          successful initialization of the I2C block, this function will have
*          set the bus speed for the master,and will have enabled the I2C Master
*          block.\n
*
* \param  baseAddr   Base Address is the Memory address of the I2C instance used \n
* \param  inputClk   It is the clock fed to the I2C instance.\n
* \param  scaledClk  It is scaled value of inputclk that is required for I2C,
*                       it should be between 6.7 to 13.3 MHz.\n
* \param  outputClk  It is the speed of clock on I2Cx_SCL pin when the I2C module \n
*                     is configured to be a master on the I2C bus.\n
*
*\return None.\n
*
**/
void I2CMasterInitExpClk(unsigned int baseAdd, unsigned int inputClk,
                         unsigned int scaledClk, unsigned int outputClk)
{
    unsigned int prescale = 0;
    unsigned int dValue = 0;
    unsigned int div = 0;

    /* Calculate the prescalar value */
    prescale = (inputClk/scaledClk) - 1;

    HWREG(baseAdd + I2C_ICPSC) = prescale;

    switch (prescale) 
    {
        case 0:
            dValue = 7;
        break;
        case 1:
            dValue = 6;
        break;
        default:
            dValue = 5;
        break;
    }

    div = scaledClk/outputClk;
    div -= (2*dValue);

    HWREG(baseAdd + I2C_ICCLKL)= div/2;
    HWREG(baseAdd + I2C_ICCLKH) = div - HWREG(baseAdd + I2C_ICCLKL);
    
    return;
}

/**
* \brief   Enables the I2C module.This will bring the I2C module out of local 
*           reset.\n
*
* \param  baseAddr Base Address is the Memory address of the I2C instance used\n
* 
* \return None.\n
*
**/
void I2CMasterEnable(unsigned int baseAddr)
{
    /* Bring the I2C module out of reset */
    HWREG(baseAddr + I2C_ICMDR) |= I2C_ICMDR_IRS;

    /* Set the backward compatibility mode off, for proper interruption */
    HWREG(baseAddr + I2C_ICEMDR) &= ~I2C_ICEMDR_BCM;
}

/**
* \brief  Disables the I2C Module.This will put the I2C module in local reset.
*
* \param  baseAddr Base Address is the Memory address of the I2C instance used
* 
*  \return None.
*
**/
void I2CMasterDisable(unsigned int baseAddr)
{
    /* Bring the I2C module out of reset */
    HWREG(baseAddr + I2C_ICMDR) &= ~(I2C_ICMDR_IRS);
}

/**
* \brief  Enables the interrupt when I2C is in Master mode.Enables the indicated
*          I2C interrupt sources.Only the sources that are enabled can be 
*          reflected to the processor interrupt; disabled sources have no effect on
*          the processor.\n
*
* \param baseAddr is the Memory address of the I2C instance used\n
* \param intFlag is the Mask which Enables the interrupt source\n
*
*        intFlag can take following values.\n
*
*               I2C_INT_ARBITRATION_LOST     - Arbitration-lost interrupt\n
*                                              
*               I2C_INT_NO_ACK               - No-acknowledgment interrupt\n
*                                              
*               I2C_INT_ADRR_READY_ACESS     - No-acknowledgment interrupt\n
*                                              
*               I2C_INT_DATA_RECV_READY      - Receive-data-ready interrupt\n
*                                              
*               I2C_INT_DATA_TRANSMIT_READY  - Transmit-data-ready interrupt\n
*                                              
*               I2C_INT_STOP_CONDITION       - Stop condition interrupt\n
*                                              
*               I2C_INT_ADRR_SLAVE            -Address-as-slave interrupt\n
*                                              
*
* \return None.\n
*
**/
void I2CMasterIntEnableEx(unsigned int baseAddr, unsigned int intFlag)
{
    /*Enable the master interrupt.*/
    HWREG(baseAddr + I2C_ICIMR) |= intFlag; 
}
/**
* \brief  Enables individual interrupt sources when I2C is in Slave mode.
*          Enables the indicated I2C interrupt sources.  Only the sources that
*          are enabled can be reflected to the processor interrupt; disabled sources
*          have no effect on the processor\n
* 
* \param baseAddr is the Memory address of the I2C instance used\n
* \param intFlag is the bit mask of the interrupt sources to be enabled.\n
* 
*        intFlag can take following values\n
*                        
*               I2C_INT_ARBITRATION_LOST     - Arbitration-lost interrupt\n
*                                              
*               I2C_INT_NO_ACK               - No-acknowledgment interrupt\n
*                                              
*               I2C_INT_ADRR_READY_ACESS     - No-acknowledgment interrupt\n
*                                              
*               I2C_INT_DATA_RECV_READY      - Receive-data-ready interrupt\n
*                                              
*               I2C_INT_DATA_TRANSMIT_READY  - Transmit-data-ready interrupt\n
*                                              
*               I2C_INT_STOP_CONDITION       - Stop condition interrupt \n
*                                             
*               I2C_INT_ADRR_SLAVE           - Address-as-slave interrupt\n
* 
*\return None.
* 
**/
void I2CSlaveIntEnableEx(unsigned int baseAddr, unsigned int intFlag)
{
    HWREG(baseAddr + I2C_ICIMR) |= intFlag;
}

/**
* \brief  Disables the interrupt when I2C is in Master mode.Disables the 
*          indicated I2C interrupt sources.  Only the sources that are 
*          enabled can be reflected to the processor interrupt;disabled sources
*          have no effect on the processor.\n
*
*
* \param baseAddr is the Memory address of the I2C instance used\n
* \param intFlag is the bit mask of the interrupt sources to be dislabled.\n
*  
* intFlag can take follwing values\n
*
*                I2C_INT_ARBITRATION_LOST      - Arbitration-lost interrupt\n
*                                               
*                I2C_INT_NO_ACK                - No-acknowledgment interrupt\n
*                                               
*                I2C_INT_ADRR_READY_ACESS      - No-acknowledgment interrupt\n
*                                               
*                I2C_INT_DATA_RECV_READY       - Receive-data-ready interrupt\n
*                                                
*                I2C_INT_DATA_TRANSMIT_READY   - Transmit-data-ready interrupt\n
*                                               
*                I2C_INT_STOP_CONDITION        - Stop condition interrupt \n
*                                                
*                I2C_INT_ADRR_SLAVE            - Address-as-slave interrupt\n
* \return None.\n
*
**/
void I2CMasterIntDisableEx(unsigned int baseAddr, unsigned int intFlag)
{
    HWREG(baseAddr + I2C_ICIMR) &= ~(intFlag);
}
/**
* \brief  Disables individual interrupt sources when I2C is in Slave mode.
*`          Disables the indicated I2C Slave interrupt sources.  Only the sources
*          that are enabled can be reflected to the processor interrupt; 
*          disabled sources have no effect on the processor.\n
*
* \param baseAddr is the Memory address of the I2C instance used \n
* \param intFlag is the bit mask of the interrupt sources to be disabled.\n
*
* intFlag is can take following values.\n
*                        
*               I2C_INT_ARBITRATION_LOST     - Arbitration-lost interrupt\n
*                                              
*               I2C_INT_NO_ACK               - No-acknowledgment interrupt\n
*                                              
*               I2C_INT_ADRR_READY_ACESS     - No-acknowledgment interrupt\n
*                                              
*               I2C_INT_DATA_RECV_READY      - Receive-data-ready interrupt\n
*                                              
*               I2C_INT_DATA_TRANSMIT_READY  - Transmit-data-ready interrupt\n
*                                              
*               I2C_INT_STOP_CONDITION       - Stop condition interrupt\n
*                                              
*               I2C_INT_ADRR_SLAVE           - Address-as-slave interrupt\n
*                                              
* \return None.\n
*
**/
void I2CSlaveIntDisableEx(unsigned int baseAddr, unsigned int intFlag)
{
    HWREG(baseAddr + I2C_ICIMR) &= ~(intFlag);
}

/**
* \brief  This API can be used to determine interrupt status for the I2C
*          in Master-Mode by reading interrupt status register i.e ICSTR.
*          For more information on ICSTR refer to SPRUFV4.\n
*
* \param baseAddr is the Memory address of the I2C instance used \n
* 
* \returns the interrupt status when I2C is in Master Mode.\n
*
**/
unsigned int I2CMasterIntStatus(unsigned int baseAddr)
{
    unsigned int rStatus;

    rStatus = HWREG(baseAddr + I2C_ICSTR);

    return rStatus;
}

/**
* \brief  This API determine interrupt status for the I2C in Slave-Mode
*         by reading interrupt status register i.e ICSTR.For more information
*         on ICSTR refer to SPRUFV4.\n
*
* \param baseAddr is the Memory address of the I2C instance used \n
*  
* \returns the interrupt status for the when I2C is in Slave mode.\n
*
**/
unsigned int I2CSlaveIntStatus(unsigned int baseAddr)
{
    unsigned int rStatus;

    rStatus = HWREG(baseAddr + I2C_ICSTR);

    return rStatus;
}

/**
* \brief  This API determine the status of any one of the bit in interrupt \n
*          status register.where intFlag is Mask of status bit needs to read.\n
* 
* \param baseAddr is the Memory address of the I2C instance used\n
* \param intFlag is Mask of status bit needs to read.\n
*
* \returns the interrupt status of the requested bit.\n
**/
unsigned int I2CSlaveIntStatusEx(unsigned int baseAddr, unsigned int intFlag)
{
    intFlag &= HWREG(baseAddr + I2C_ICSTR);
        
    return intFlag;
}

/**
* \brief  This API clears  the status of any one of the bit in interrupt status
*         register.where intFlag is Mask of status bit needs to be clear.\n
*
* \param baseAddr is the Memory address of the I2C instance used\n
* \intFlag is the mask of status bit to be cleared.\n
*
*        intFlag can take following values\n
*        
*                I2C_CLEAR_NO_ACK          - No-acknowledgment interrupt
*                                            flag bit.\n
*                I2C_CLEAR_ADDR_READY      - Register-access-ready interrupt
*                                            flag\n
*                I2C_CLEAR_DATA_READY      - Receive-data-ready interrupt
*                                            flag bit\n
*                I2C_CLEAR_TRANSMIT_READY  - Transmit-data-ready interrupt
*                                            flag bit.\n
*                I2C_CLEAR_STOP_CONDITION  - Stop condition detected bit.\n
*                I2C_CLEAR_BUS_BUSY        - Bus busy bit.\n
*                I2C_CLEAR_NO_ACK_SENT     - No-acknowledgment sent bit\n
*                I2C_CLEAR_ARBITARTION_LOST-Arbitration-lost interrupt flag b\n
*
* \return None.\n
**/
void I2CMasterIntClearEx(unsigned int baseAddr, unsigned int intFlag)
{
    HWREG(baseAddr + I2C_ICSTR) = intFlag;
}

/**
* \brief  This API clears the status of any one of the bit in interrupt status\n
*      register.where intFlag is Mask of status bit needs to be clear.\n
*
* \param  baseAddr is the Memory address of the I2C instance used\n
* \param  intFlag is the mask of interrupt source to be cleared.\n
*
*         intFlag can take following values\n
*
*               I2C_CLEAR_NO_ACK        - No-acknowledgment interrupt flag bit.\n
*               I2C_CLEAR_ADDR_READY    - Register-access-ready interrupt flag
*                                         bit.\n
*               I2C_CLEAR_DATA_READY    - Receive-data-ready interrupt flag bit\n
*               I2C_CLEAR_TRANSMIT_READY- Transmit-data-ready interrupt flag bit.\n
*               I2C_CLEAR_STOP_CONDITION- Stop condition detected bit.\n
*               I2C_CLEAR_BUS_BUSY      - Bus busy bit.\n
*               I2C_CLEAR_NO_ACK_SENT   - No-acknowledgment sent bit\n
*               I2C_CLEAR_ARBITARTION_LOST-Arbitration-lost interrupt flag \n
*
* 
* \return None.\n
*
**/
void I2CSlaveIntClearEx(unsigned int baseAddr, unsigned int intFlag)
{
    HWREG(baseAddr + I2C_ICSTR) = intFlag;
}

/**
* \brief Sets the address that the I2C Master will place on the bus.\n
*
* \param baseAddr is the Memory address of the I2C instance used \n
* \param slaveAddr slave address \n
*
* \return None \n
**/
void I2CMasterSlaveAddrSet(unsigned int baseAddr, unsigned int slaveAddr)
{
    /*Set the address of the slave with which the master will communicate.*/
    HWREG(baseAddr + I2C_ICSAR) = slaveAddr;
}

/**
* \brief   This function returns the indication of whether or not I2c bus is busy
*
* \param   baseAddr is the Memory address of the I2C instance used
*   
* \return  non zero of  the bus is busy, zero otherwise 
* 
**/
unsigned int  I2CMasterBusBusy(unsigned int baseAddr)
{
    return ((HWREG(baseAddr + I2C_ICSTR) & I2C_ICSTR_BB));
}

/**
* \brief   This function checks if indeed I2c operation has completed.
*
* \param   baseAddr is the Memory address of the I2C instance used\n
*   
* \return   Non-Zero if the I2C peripheral operation is not complete
*           0 if complete
* 
* \note    This is different from bus busy. This API checks if the peripheral is 
*          ready to be used for next transaction.
**/
unsigned int  I2CMasterIsBusy(unsigned int baseAddr)
{
    return ((HWREG(baseAddr + I2C_ICMDR) & I2C_ICMDR_MST));
}

/**
* \brief  This API is use to configure the Mode of operation of I2C controller,
*           like setting I2C to Master-Transmitter or Master-Reciever Slave-Transmitter
*           or Slave-reciver,addressing Mode 7bit or 10bit etc for more information on
*         I2c Mode register i.e ICMDR referto SPRUFV4 document,which is a I2C
*         peripheral manual.\n
*
* 
* \param baseAddr is the Memory address of the I2C instance used.\n
* \param cmd is the command to be issued to the I2C Master module\n
*
* cmd can take following values.\n
*               I2C_CFG_MST_TX            -     This command configures I2C to
                                                Master-trasmiter Mode \n.
*
*               I2C_CFG_MAST_STOP_TX      -     This command configures I2C to
*                                               Master-trasmiter generates stop
*                                               conditon when internal data 
*                                               counts to 0 \n
*
*               I2C_CFG_MST_RX            -     This command configuers I2C to
*                                               Master-Receiver Mode.
*
*               I2C_CFG_NACKMOD           -     This command set the NACKMOD bit
*                                               in ICMDR to send NACK bit to
*                                               slave device to stop 
                                                transmission of data.\n
*
*               I2C_CFG_STOP              -     This command sets STP bit in
*                                               ICMDR manualy to generate STOP
*                                               condition.\n
*
*
*               I2C_CFG_7BIT_ADDR_MODE    -     This command configures I2C in
*                                               to 7 bit addressing mode.\n
* 
*               I2C_CFG_10BIT_ADDR_MODE   -     This command configures I2C in
*                                               to 10 bit addressing mode.\n
* 
*               I2C_CFG_FREE_FORMAT_DATA  -     This command configures I2C in
*                                               to free format addressing mode.\n
*
*               I2C_CFG_SLAVE_STT         -     This command configures I2C to
*                                               Slave mode and sets STT bit in
*                                               ICSTR,if STT bit is not set 
*                                               slave not responds to commands
*                                               from master. \n
*
*               I2C_CFG_REPEAT_MODE       -    This command configures I2C to
*                                              Repeat mode. In Repeat mode data
*                                              words are continously transmited
*                                              /received by I2C until STP bit
*                                              is manually set to 1,regardless
*                                              of the value in ICCNT. \n
*
*              \return None
**/
void I2CMasterControl(unsigned int baseAddr, unsigned int cmd)
{
    /* Since, the IRS bit needs to be set, to bring the module out
     * of local reset, we do that here, every time
     */
    HWREG(baseAddr + I2C_ICMDR) = cmd;
    HWREG(baseAddr + I2C_ICMDR) |= I2C_ICMDR_IRS;
}

/**
* \brief  This API is used to start a I2C transaction on the bus. This API must
*         be called after all the configuration for the i2c module is done and after
*        bringing I2C out of local reset
* 
* \param baseAddr is the Memory address of the I2C instance used.\n
*
* \return None
**/
void I2CMasterStart(unsigned int baseAddr)
{
    HWREG(baseAddr + I2C_ICMDR) |= I2C_ICMDR_STT;
}

/**
* \brief  This API is used to stop a I2C transaction on the bus.\n
*         This API must be used in case a deliberate STOP needs to be sent
*         on the bus
* 
* \param baseAddr is the Memory address of the I2C instance used.\n
*
* \return None
**/
void I2CMasterStop(unsigned int baseAddr)
{
    HWREG(baseAddr + I2C_ICMDR) |= I2C_ICMDR_STP;
}

/**
* \brief  This API is used to clear a specific status bit(s).\n
* 
* \param  baseAddr is the Memory address of the I2C instance used.\n
* \param  status   contains mask of status bit(s) to be cleared.\n
*
* \return None
**/
void I2CStatusClear(unsigned int baseAddr, unsigned int status)
{
    /* ICSTR bits are write-1-to-clear. Hence we can just write
     * the status mask here, as writing zero will have no effect
     */
    HWREG(baseAddr + I2C_ICSTR) = status;
}

/**
* \brief  This function indicates error if occured in I2C operation.\n
*
* \param  baseAddr is the Memory address of the I2C instance used.\n
*  
* \returns 1 is error has because of above reasons,otherwise;return 0
* i.e (error has not occured);\n
*
**/
unsigned int I2CMasterErr(unsigned int baseAddr)
{
    unsigned int err;

    err = HWREG(baseAddr + I2C_ICSTR) & (I2C_ICIMR_AL | I2C_ICIMR_NACK |
                                                       I2C_ICSTR_RSFULL);
    return err;
}

/**
* \brief  This function Transmits a byte from the I2C in Master mode.\n
* 
* \param baseAddr is the Memory address of the I2C instance used.\n
* \param data data to be transmitted from the I2C Master\n
* 
* \return None.\n
*
**/
void I2CMasterDataPut(unsigned int baseAddr, unsigned char data)
{
     /*write data to be transmited to Data transmit register */
    HWREG(baseAddr + I2C_ICDXR) = data;
}

/**
*  \brief  This Receives a byte that has been sent to the I2C in Master mode.\n
* 
*  \param baseAddr is the Memory address of the I2C instance used.\n
* 
*  \return Returns the byte received from by the I2C in Master mode.\n
*
**/
unsigned int I2CMasterDataGet(unsigned int baseAddr)
{
    unsigned int rData;

    rData = HWREG(baseAddr + I2C_ICDRR);
    return rData;
}
/**
* \brief This Transmits a byte from the I2C in Slave mode.\n
* 
* \param baseAddr is the Memory address of the I2C Slave module.\n
* \param data data to be transmitted from the I2C in Slave mode.\n
* 
* \return None.\n
*
**/

void I2CSlaveDataPut(unsigned int baseAddr, unsigned char data)
{
    HWREG(baseAddr + I2C_ICDXR)= data;
}

/**
* \brief  This function Receives a byte that has been sent to the I2C in Slave
*          mode.\n
* 
* \param baseAddr is the Memory address of the I2C instace used.\n
* 
* \return Returns the byte received from by the I2C in Slave mode\n
* 
**/
unsigned int I2CSlaveDataGet(unsigned int baseAddr)
{
    unsigned int rData;

    rData = HWREG(baseAddr + I2C_ICDRR);

    return rData;
}
/**
* \brief  This API set I2C data count register (ICCNT) with count value.The value
*          in the I2C data count register indicate how many data words to transfer
*          when the I2C is configured as a  master-transmitter and repeat mode is off.\n

* \param baseAddr is the Memory address of the I2C instance used.\n
* \param count is value which is set to I2C data count register.\n
*
* \return None.\n
**/
void I2CSetDataCount(unsigned int baseAddr, unsigned int count)
{
    HWREG(baseAddr + I2C_ICCNT)= count;
}

/**
* \brief -This API set its own address.which is used in loop back mode.\n
*
* \param baseAddr is the Memory address of the I2C instance used.\n
*
* \return None.
* **/
void I2COwnAddressSet(unsigned int baseAddr)
{
    HWREG(baseAddr + I2C_ICOAR)= baseAddr;
}
/**
* \brief This API returs INTCODE which determines,what event caused interrupt.
*
* \param baseAddr is the Memory address of the instance used.
*
* \return None.
**/
unsigned int I2CInterruptVectorGet(unsigned int baseAddr)
{
    return (HWREG(baseAddr + I2C_ICIVR) & I2C_ICIVR_INTCODE);
}
/**
* \brief This API returns address of slave.
*
* \param baseAddr is the Memory address of the instance used.\n
*
* \return Returns the current slave adddress
**/
unsigned int I2CSlaveAddressGet(unsigned int baseAddr)
{
    return (HWREG(baseAddr + I2C_ICSAR) & I2C_ICSAR_SADDR);
}

/**
* \brief This API disables both transmit and receiption event of I2C.
*
* \param baseAddr is the Memory address of the instance used.
*
* \return None.
*
**/
void I2CDMATxRxEventDisable(unsigned int baseAddr)
{
    HWREG(baseAddr + I2C_ICDMAC) = I2C_TX_RX_EVENT_DISABLE;
}

/**
* \brief This API Enables transmit event of I2C
*
* \param baseAddr is the Memory address of the instance used.
*
* \return None.
*
**/
void I2CDMATxEventEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + I2C_ICDMAC) |= I2C_ICDMAC_TXDMAEN; 
}

/**
* \brief This API Enables receiption event of I2C
*
* \param baseAddr is the Memory address of the instance used.
*
* \return None.
*
**/
void I2CDMARxEventEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + I2C_ICDMAC) |= I2C_ICDMAC_RXDMAEN;
}

/**
* \brief This API disables transmit event of I2C
*
* \param baseAddr is the Memory address of the instance used.
*
* \param flag     is the value which disables the transmit event of I2C
* 
* \return None.
*
**/
void I2CDMATxEventDisable(unsigned int baseAddr)
{
    HWREG(baseAddr + I2C_ICDMAC) &= ~I2C_ICDMAC_TXDMAEN;
}

/**
* \brief This API disables receiption event of I2C
*
* \param baseAddr is the Memory address of the instance used.
*
* \param flag     is the value which disables the receiption event of I2C
* 
* \return None.
*
**/
void I2CDMARxEventDisable(unsigned int baseAddr)
{
    HWREG(baseAddr + I2C_ICDMAC) &= ~I2C_ICDMAC_RXDMAEN;
}
