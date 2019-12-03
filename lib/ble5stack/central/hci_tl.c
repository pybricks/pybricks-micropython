/* --COPYRIGHT--,BSD
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/

/* Standard Includes */
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/* TI-Driver Includes */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>

/* ble5stack Includes */
#include "hci_tl.h"
#include "gap.h"


/* HCI RX States */
typedef enum
{
    HCI_READ_PACKET_TYPE = 0,
    HCI_READ_EVENT_CODE,
    HCI_READ_PLD,
    HCI_READ_MSG,
    HCI_READ_IDLE
} readHCIState_t;

/* HCI Packet information */
#define HCI_READ_PACKET_TYPE_LEN                     0x01
#define HCI_READ_EVENT_CODE_LEN                      0x01
#define HCI_READ_PLD_LEN                             0x01

/* Enable or disable power management */
#define POWER_SAVING 0

/* HCI Event Packet state machine info */
readHCIState_t readHCIState = HCI_READ_EVENT_CODE;
uint16_t readNumBytes;
uint8_t HCImsgIndex;

/* HCI transport buffers */
uint8_t HCI_txBuffer[TX_BUFFER_SIZE];
uint8_t HCI_rxBuffer[RX_BUFFER_SIZE];

/* Handle for HCI UART port */
UART_Handle uartHCIHandle = NULL;

/* Task configuration */
static pthread_t hciRxTask;

/* BLE package received semaphore */
sem_t hciRxEventSem;

#if (POWER_SAVING == 1)
/* Indexes for pin configurations in GPIO configuration array */
#define MRDY_PIN      2
#define SRDY_PIN      3

/* Enable MRDY by setting it low */
#define MRDY_ENABLE()     GPIO_write(MRDY_PIN, 0);
/* Disable MRDY by setting it high */
#define MRDY_DISABLE()    GPIO_write(MRDY_PIN, 1);

/* Semaphores to block master until slave is ready for transfer */
sem_t masterSem;
sem_t slaveSem;

/* Is transaction initiated by master device */
bool masterTransaction = false;

bool rxActive = false;
bool txActive = false;

/* Callback function for the GPIO interrupt SRDY_PIN */
void slaveReadyFxn(uint_least8_t index);
#endif


/*******************************************************************************
 *                         LOCAL FUNCTIONS
 ******************************************************************************/
static void *HCIUART_taskFxn(void *arg0);
extern void HCITask_open(void);

/*******************************************************************************
 *                        PUBLIC FUNCTIONS
 ******************************************************************************/

/*******************************************************************************
 * @fn      HCI_decodeEventHeader
 *
 * @brief   Decode the header of a received HCI Event Packet.
 *
 * @param[in]  packet  - received packet.
 *
 * @return : the decoded event header
 ******************************************************************************/
eventHeader_t HCI_decodeEventHeader(uint8_t *packet)
{
    eventHeader_t header;
    header.type = packet[0];
    header.eventCode = packet[1];
    header.dataLength = packet[2];

  switch(header.eventCode)
  {
  case HCI_COMMANDCOMPLETEEVENT:   /* Received HCI command complete event */
      header.eventOpCode = 0x00;
      header.status = packet[6];
      header.esg = NULL;
      break;
  case HCI_LE_EXTEVENT:   /* Received HCI Vendor Specific Event */
      header.eventOpCode = packet[4];
      header.eventOpCode = packet[3] + (header.eventOpCode << 8);
      header.esg = ((header.eventOpCode) & (0x0380)) >> 7;
      header.status = packet[5];
      break;
  default: /* Received other event */
      header.eventOpCode = NULL;
      header.esg = NULL;
      header.status = FAILURE;
      break;
  }
    return (header);
}

/*******************************************************************************
 * @fn      HCITask_open
 *
 * @brief   Open the HCI Task
 *
 * @param[in]  None
 *
 * @return : None
 ******************************************************************************/
void HCITask_open(void)
{
    pthread_attr_t pAttrs;
    struct sched_param priParam;
    int retc;
    int detachState;

    pthread_attr_init(&pAttrs);
    priParam.sched_priority = HCI_RX_TASK_PRIORITY;

    detachState = PTHREAD_CREATE_DETACHED;
    retc = pthread_attr_setdetachstate(&pAttrs, detachState);

    if (retc != 0)
    {
        while(1);
    }

    pthread_attr_setschedparam(&pAttrs, &priParam);

    retc |= pthread_attr_setstacksize(&pAttrs, HCI_RX_TASK_STACK_SIZE);

    if (retc != 0)
    {
        while(1);
    }

    retc = pthread_create(&hciRxTask, &pAttrs, HCIUART_taskFxn, NULL);

    if (retc != 0)
    {
        while(1);
    }
}

/*******************************************************************************
 * @fn      HCI_init
 *
 * @brief   Initialize the HCI transport layer
 *
 * @param[in]  hciParams - Struct containing parameters to initialize the
 *              HCI transport layer
 *
 * @return : None
 ******************************************************************************/
void HCI_init(HCI_Params *hciParams)
{
    /* Initialize the semaphore used to handle timing of received events */
    sem_init(&hciRxEventSem, 1, 0);

    /* Close UART if already open*/
    HCI_close();

    /* Open the HCI transport layer */
    HCI_open(hciParams);
}

/*******************************************************************************
 * @fn      HCI_init
 *
 * @brief   Open the HCI transport layer.
 *          If power management is enabled, then this function will initialize
 *          the GPIO pins MRDY and SRDY used to handle the handshake sequence
 *          that guarantees both devices are awake and ready to transport data.
 *
 * @param[in]  hciParams - Struct containing parameters to initialize the
 *              HCI transport layer
 *
 * @return : None
 ******************************************************************************/
void HCI_open(HCI_Params *hciParams)
{
    UART_Params uartParams;

    if ((hciParams->portType == HCI_PORT_REMOTE_UART)
            && (uartHCIHandle == NULL))
    {
        /* Create a UART with data processing off. */
        UART_Params_init(&uartParams);
        uartParams.writeDataMode = UART_DATA_BINARY;
        uartParams.readDataMode = UART_DATA_BINARY;
        uartParams.readReturnMode = UART_RETURN_FULL;
        uartParams.readMode = UART_MODE_BLOCKING;
        uartParams.writeMode = UART_MODE_BLOCKING;
        uartParams.readEcho = UART_ECHO_OFF;
        uartParams.baudRate = 115200;

        uartHCIHandle = UART_open(hciParams->remote.boardID, &uartParams);
    }

#if (POWER_SAVING == 1)
    /* Initialize the semaphores used to handle the timing between master
     * and slave device communication.
     */
    sem_init(&masterSem, 1, 0);
    sem_init(&slaveSem, 1, 0);

    /* Below we set Board_MRDY & Board_SRDY initial conditions for the 'handshake'.
    * MRDY and SRDY are ACTIVE LOW signals. Then configure the interrupt on Board_SRDY
    */
    GPIO_setConfig(MRDY_PIN, GPIO_CFG_OUTPUT | GPIO_CFG_OUT_HIGH);
    GPIO_setConfig(SRDY_PIN, GPIO_CFG_INPUT);
    GPIO_setConfig(SRDY_PIN, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setCallback(SRDY_PIN, slaveReadyFxn);
    GPIO_enableInt(SRDY_PIN);

#endif

    /* Reset the HCI transport state machine */
    HCImsgIndex = 0;
    readNumBytes = HCI_READ_PACKET_TYPE_LEN;
    readHCIState = HCI_READ_PACKET_TYPE;

    /* Open the HCI Task */
    HCITask_open();
}


/*******************************************************************************
 * @fn      HCI_close
 *
 * @brief   Close the HCI Transport layer
 *
 * @param   None
 *
 * @return  None
 ******************************************************************************/
void HCI_close(void)
{
    if (uartHCIHandle != NULL)
    {
        UART_close(uartHCIHandle);
        uartHCIHandle = NULL;
    }
}

/*******************************************************************************
 * @fn      HCI_transportHCIPacket
 *
 * @brief   Transport the HCI packet over the transport layer
 *          If power management is enabled, then the application will start
 *          the handshake sequence to ensure the slave device is ready to
 *          receive data.
 *
 * @param[in]   pPkt - The HCI packet to transport
 * @param[in]   dataLength - The length of the payload data to transport
 *
 * @return  None
 ******************************************************************************/
void HCI_transportHCIPacket(hciPacket_t *pPkt, uint8_t dataLength)
{
#if (POWER_SAVING == 1)
    masterTransaction = true;

    /* Wait until slave device is ready to receive data, and all other RX and
     * TX actions are complete */
    while ((GPIO_read(SRDY_PIN) == 0) || rxActive || txActive) {}

    txActive = true;

    /* Master device is ready to transfer. Wait for slave device to be ready */
    MRDY_ENABLE();
    sem_wait(&masterSem);

    /* Slave device is ready. Start UART write */
    UART_write(uartHCIHandle, pPkt, HCI_FRAME_SIZE);
    UART_write(uartHCIHandle, pPkt->pData, dataLength);

    /* Done transmitting. Disable master device */
    masterTransaction = false;
    txActive = false;
    MRDY_DISABLE();

#else
    UART_write(uartHCIHandle, pPkt, HCI_FRAME_SIZE);
    UART_write(uartHCIHandle, pPkt->pData, dataLength);
#endif
}

/*******************************************************************************
 * @fn      HCI_mallocFrame
 *
 * @brief   Allocate memory for the packet
 *
 * @param[in]   dataLength - The length of the payload data to transport
 *
 * @return  hciPacket_t - HCI packet that points to the newly allocated memory
 ******************************************************************************/
hciPacket_t * HCI_mallocFrame(uint8_t dataLength)
{
    hciPacket_t *pMsg;

    /* Try to allocate HCI packet structure */
    pMsg = (hciPacket_t *) malloc(sizeof(hciPacket_t));
    if (pMsg == NULL)
    {
        return NULL;
    }

    if (dataLength != 0)
    {
        /* Try to allocate data, free structure if fail */
        pMsg->pData = (uint8_t *) malloc(dataLength);
        if (pMsg->pData == NULL)
        {
            free(pMsg);
            return NULL;
        }
    }
    /* Set size */
    pMsg->dataLength = dataLength;

    return pMsg;
}

/*******************************************************************************
 * @fn      HCI_delPacket
 *
 * @brief   Deallocates memory of a HCI packet
 *
 * @param[in]   hciPacket - The length of the payload data to transport
 *
 * @return  None
 ******************************************************************************/
void HCI_delPacket(hciPacket_t *hciPacket)
{
    if (hciPacket != NULL)
    {
        free(hciPacket);
    }
}

/*******************************************************************************
 * @fn      HCI_buildHCIPacket
 *
 * @brief   Build an HCI Packet
 *
 * @param[in]   hciPacket - The length of the payload data to transport
 *
 * @return  None
 ******************************************************************************/
hciPacket_t * HCI_buildHCIPacket(uint8_t packetType, uint16_t opCode, uint8_t dataLength)
{
    hciPacket_t *pPkt;

    /* If malloc succeeds, set the rest of the struct members */
    if ((pPkt = HCI_mallocFrame(dataLength)))
    {
        pPkt->packetType = packetType;
        pPkt->opcodeLO = LO_UINT16(opCode);
        pPkt->opcodeHI = HI_UINT16(opCode);
    }

    return pPkt;
}

/*******************************************************************************
 * @fn      HCI_sendHCICommand
 *
 * @brief   Send a HCI Command Packet over the transport layer
 *
 * @param[in]   opcode - The HCI Command opcode of the packet to transport
 * @param[in]   pData - The payload data of the HCI Command packet
 * @param[in]   dataLength - The length of the payload data
 *
 * @return  HCI_StatusCodes_t status code
 ******************************************************************************/
HCI_StatusCodes_t HCI_sendHCICommand(uint16_t opcode, uint8_t *pData, uint8_t dataLength)
{
    hciPacket_t *pPkt;
    HCI_StatusCodes_t status;

    /* Allocated an empty HCI packet */
    pPkt = HCI_buildHCIPacket(HCI_CMD_PACKET, opcode, dataLength);

    /* If allocation failed, then return status FAILURE */
    if (pPkt == NULL)
    {
        status = FAILURE;
    }
    else
    {
        status = SUCCESS;

        /* Copy pointer data */
        memcpy(pPkt->pData, pData, dataLength);

        /* Send HCI packet */
        HCI_transportHCIPacket(pPkt, dataLength);
    }

    /* Free allocated memory */
    HCI_delPacket(pPkt);

    return status;
}

/*******************************************************************************
 * @fn      HCI_ResetCmd
 *
 * @brief   Issue a hard or soft system reset. A hard reset is caused by
 *          setting the SYSRESET bit in the System Controller Reset Control
 *          register. The soft reset is currently not supported on the CC264x.
 *
 * @param[in] type - Reset type.
 *
 * @return HCI_StatusCodes_t
 ******************************************************************************/
HCI_StatusCodes_t HCI_ResetCmd(uint8_t type)
{
    HCI_StatusCodes_t status;
    uint8_t pData[1];

    pData[0] = type;

    status = HCI_sendHCICommand(HCI_EXT_RESETSYSTEMCMD, pData, 1);

    return status;
}

/*******************************************************************************
 * @fn      HCI_waitForEvent
 *
 * @brief   Wait to receive an HCI event over the transport layer
 *
 * @param   None
 *
 * @return  None
 ******************************************************************************/
int HCI_waitForEvent(void)
{
    return (sem_wait(&hciRxEventSem));
}

/*******************************************************************************
 * @fn      HCIUART_taskFxn
 *
 * @brief   Task that handles receiving the HCI packets over the UART
 *          transport layer. If power management is enabled, then the
 *          task will pend on a semaphore until the slave device indicates
 *          it is ready to transport data. If power management is disabled,
 *          then the task blocks on a UART_read.
 *
 * @param   arg0 not used.
 *
 * @return  None.
 ******************************************************************************/
static void *HCIUART_taskFxn(void *arg0)
{
    static uint8_t payLoadLen = 0;

    while (1)
    {
#if (POWER_SAVING == 1)

        /* Wait until slave device is ready to transmit data, then enable master
         * device to receive */
        if (!rxActive)
        {
            sem_wait(&slaveSem);
            rxActive = true;
            MRDY_ENABLE();
        }
#endif

        /* Block until data is available */
        UART_read(uartHCIHandle, &HCI_rxBuffer[HCImsgIndex], readNumBytes);

        switch (readHCIState)
        {
        case HCI_READ_PACKET_TYPE:
            if (HCI_rxBuffer[0] == HCI_EVENT_PACKET)
            {
                /* Received PACKET_TYPE, read EVENT_CODE next */
                HCImsgIndex++;
                readNumBytes = HCI_READ_EVENT_CODE_LEN;
                readHCIState = HCI_READ_EVENT_CODE;
            }
            break;

        case HCI_READ_EVENT_CODE:
        {
            /* Received EVENT_CODE, read PLD next */
            HCImsgIndex++;
            readNumBytes = HCI_READ_PLD_LEN;
            readHCIState = HCI_READ_PLD;
        }
            break;

        case HCI_READ_PLD:
        {
            /* Determine length of remainder of packet */
            payLoadLen = HCI_rxBuffer[2];

            /* Read payload bytes */
            HCImsgIndex++;
            readNumBytes = payLoadLen;
            readHCIState = HCI_READ_MSG;
        }
            break;

        case HCI_READ_MSG:
        {
#if (POWER_SAVING == 1)
            MRDY_DISABLE();
            rxActive = false;
#endif

            /* Post sem that event has been received */
            sem_post(&hciRxEventSem);

            /* Reset State. Full packet has been read or error has occurred */
            HCImsgIndex = 0;
            readNumBytes = HCI_READ_PACKET_TYPE_LEN;
            readHCIState = HCI_READ_PACKET_TYPE;
        }
            break;

        case HCI_READ_IDLE:
        default:
            /* Should not get here. If so reset read state */
            readHCIState = HCI_READ_PACKET_TYPE;
            break;
        }
    }
}

#if (POWER_SAVING == 1)
/*******************************************************************************
 * @fn      slaveReadyFxn
 *
 * @brief   Callback function for the GPIO interrupt SRDY_PIN
 *
 * @param   None
 *
 * @return  None
 ******************************************************************************/
void slaveReadyFxn(uint_least8_t index)
{
    if (masterTransaction)
    {
        sem_post(&masterSem);

    }
    else
    {
        if (!rxActive && !txActive)
        {
            sem_post(&slaveSem);
        }
    }
}
#endif // POWER_SAVING = 1
