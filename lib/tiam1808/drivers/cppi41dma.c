/**
 * \file  cppi41dma.c
 *
 * \brief This file contains the API definitions for initializing and
 *        configuring CPPI DMA engine.
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
#include "hw_types.h"
#include "hw_usb.h"
#include "debug.h"
#include "cppi41dma.h"
/* Driver APIs */
#include "usb.h"
#ifdef _TMS320C6X
#include "dspcache.h"
#else
#include "cp15.h"
#endif
#include "uartStdio.h"

/*****************************************************************************
**                 STATIC FUNCTIONs
******************************************************************************/

static void Cppi41DmaModeSet(unsigned short usbDevInst, endpointInfo *epInfo);
static void Cppi41DmaInitBdPool();
static unsigned int Cppi41DmaReadCompletionQueue(unsigned short usbDevInst,
													unsigned int queueNum);

static void Cppi41DmaConfigRxhostPacket(unsigned short usbDevInst, unsigned short chan,
										unsigned int rxSubmitqA, unsigned int rxSubmitqB);

static void Cppi41DmaConfigRxCompletionQueue(unsigned short usbDevInst,
												unsigned short rxChan,unsigned int rxCompq);
static void Cppi41DmaConfigTxCompletionQueue(unsigned short usbDevInst,
												unsigned short txChan,unsigned int txCompq);

static void Cppi41DmaClearINTD0Status(unsigned short usbDevInst);

static void Cppi41DmaControlScheduler(unsigned short usbDevInst,
												unsigned short enableDisable,
													unsigned short numOfentries);

static void Cppi41DmaProcessBD( unsigned short usbDevInst, hostPacketDesc *current_bd,
											unsigned short direction, unsigned char * buffadd,
												unsigned int length, unsigned int ulEndpoint);
static void cppi41DmaInitBuffer();

/*****************************************************************************
**                  VARIABLES AND BUFFERS
******************************************************************************/

/*Link RAM 0 area declaration */
static unsigned int linking_ram0[LINK_RAM_SIZE];

/*Descriptor region 0 declaration */
unsigned int region0DescriptorSpace[DESC_REGION_SIZE];

/*CPPI info structure */
cppi41DmaInfo cppiInfo;

/*Data Buffers for DMA data transfer */
#if defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment=(32)
unsigned char dBuffer[CPDMA_NUMOF_BUFFERS][USB_PACKET_LENGTH];
#elif defined __TMS470__ || defined _TMS320C6X
#pragma DATA_ALIGN(dBuffer, 32);
unsigned char dBuffer[CPDMA_NUMOF_BUFFERS][USB_PACKET_LENGTH];
#else
unsigned char dBuffer[CPDMA_NUMOF_BUFFERS][USB_PACKET_LENGTH]__attribute__((aligned(32)));
#endif

/*Buffer management structre */
typedef struct
{
	unsigned int *dataBuffer;
	unsigned short usedFlag;
	unsigned short nBlocks;
}dmaDataBuffer;

#if defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment=(32)
dmaDataBuffer dmaBuffer[CPDMA_NUMOF_BUFFERS];
#elif defined __TMS470__ || defined _TMS320C6X
#pragma DATA_ALIGN(dmaBuffer, 32);
dmaDataBuffer dmaBuffer[CPDMA_NUMOF_BUFFERS];
#else
dmaDataBuffer dmaBuffer[CPDMA_NUMOF_BUFFERS]__attribute__((aligned(32)));
#endif

/* Converts from an endpoint specifier to the offset of the endpoint's
 * control/status registers. */
#define EP_OFFSET(Endpoint)     (Endpoint - 0x10)
#define QUEUE_OFFSET(queue) 	(queue << 4)
#define CHANNEL_OFFSET(chan)	(chan << 5)

/*****************************************************************************
**                   FUNCTION DEFINITIONS
******************************************************************************/

/**
* \brief	  This API initializes the DMA. This API should be called by the application
*		  API .  API populates cppi Info structure according to the information
*		  passed from the application. This information is used by this API and all other
*		  APIs. This API configures Channels queues and schedulers as per the
*		  information passed from the application.
*
* \param	  usbDevIns :- This is the USB device instance
*
*\param	  epInfo:- End point info structure
*
*\param	  numOfEndPoimts:- Total Number of endpoints to be configured(RX+TX)
*
* \return	  None.
*
**/
void Cppi41DmaInit(unsigned short usbDevInst, endpointInfo *epInfo,
						unsigned short numOfEndPoimts)
{
	/* This function need to be changed for porting to different SoC ,
	 * to initialise base addresses and other details. */

	unsigned int descAddress = 0;
	unsigned short i= 0;
	usbInstance *usbInstance;
#if defined (am335x_15x15) || defined(am335x)
	unsigned short j = 0;
#endif

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	/*Populate the CPPI info structure */
	cppiInfo.head_bd = NULL;
	cppiInfo.tail_bd = NULL;

#if defined (am335x_15x15) || defined(am335x)
	for(i =0; i < 15; i++)
	{
		/*TX Completion queues */
		cppiInfo.txCompletionq[i]= TX_COMPQ1 + i;
		/*RX Completion queues */
		cppiInfo.rxCompletionq[i]= RX_COMPQ1 + i;

		/*TX submit queues */
		for(j = 0; j < 2; j++)
		{
			cppiInfo.txSubmitq[i][j]=TX_SUBMITQ1 + (i *2) + j;
		}
	}

#else
	/*TX Completion queues */
	cppiInfo.txCompletionq[0]= TX_COMPQ1;
	cppiInfo.txCompletionq[1]= TX_COMPQ2;
	cppiInfo.txCompletionq[2]= TX_COMPQ1;
	cppiInfo.txCompletionq[3]= TX_COMPQ2;

	/*RX Completion queues */
	cppiInfo.rxCompletionq[0]= RX_COMPQ1;
	cppiInfo.rxCompletionq[1]= RX_COMPQ2;
	cppiInfo.rxCompletionq[2]= RX_COMPQ1;
	cppiInfo.rxCompletionq[3]= RX_COMPQ2;

	/*TX submit queues */
	cppiInfo.txSubmitq[0][0]=TX_SUBMITQ1;
	cppiInfo.txSubmitq[0][1]=TX_SUBMITQ2;
	cppiInfo.txSubmitq[1][0]=TX_SUBMITQ3;
	cppiInfo.txSubmitq[1][1]=TX_SUBMITQ4;
	cppiInfo.txSubmitq[2][0]=TX_SUBMITQ5;
	cppiInfo.txSubmitq[2][1]=TX_SUBMITQ6;
	cppiInfo.txSubmitq[3][0]=TX_SUBMITQ7;
	cppiInfo.txSubmitq[3][1]=TX_SUBMITQ8;
#endif

	/*USB base address and OTG basse adress  for USB instance 0 */
	cppiInfo.usbInst[0].usbBaseAddress = USB0_BASE;
	cppiInfo.usbInst[0].otgBaseAddress = USB_OTGBASE;

	/*This function initializes the buffer Management system */
	cppi41DmaInitBuffer();

	/* This loop populates the RX and TX queues(both submission and completion queues),
	 * channels and dma mode for each end point */
	for(i= 0;i< numOfEndPoimts;i++)
	{
		if((epInfo + i)->direction == CPDMA_DIR_RX)
		{
			usbInstance->rxEndPoint[(epInfo + i)->endPoint].channel =
												   (epInfo + i)->endPoint - 1;

			usbInstance->rxEndPoint[(epInfo + i)->endPoint].complettionq =
									 cppiInfo.rxCompletionq[(epInfo + i)->endPoint - 1];

			usbInstance->rxEndPoint[(epInfo + i)->endPoint].submitq =
													(epInfo + i)->endPoint - 1;

			usbInstance->rxEndPoint[(epInfo + i)->endPoint].mode =
														(epInfo + i)->dmaMode;
		}
		else
		{
			usbInstance->txEndPoint[(epInfo + i)->endPoint].channel =
													(epInfo + i)->endPoint - 1;
			usbInstance->txEndPoint[(epInfo + i)->endPoint].complettionq =
									cppiInfo.txCompletionq[(epInfo + i)->endPoint - 1];

			usbInstance->txEndPoint[(epInfo + i)->endPoint].submitq =
									 cppiInfo.txSubmitq[(epInfo + i)->endPoint - 1][0];

			usbInstance->txEndPoint[(epInfo + i)->endPoint].mode =
														(epInfo + i)->dmaMode;
		}
	}

	/* Set  mode for each end pont*/
	for(i = 0;i < numOfEndPoimts; i++)
		Cppi41DmaModeSet(usbDevInst, &epInfo[i]);

	/*Link Ram setup	 */
	HWREG(usbInstance->otgBaseAddress + CPDMA_LRAM_0_BASE) = (unsigned int)&linking_ram0[0];
	HWREG(usbInstance->otgBaseAddress + CPDMA_LRAM_0_SIZE) = sizeof(linking_ram0);
	HWREG(usbInstance->otgBaseAddress + CPDMA_LRAM_1_BASE) = 0x0;

	/*Queue region setup */
	/*The Descriptor region address is aligned for 64 Byte access */
	descAddress = (unsigned int)region0DescriptorSpace;
	/*Add 64 to the starting address */
	descAddress = descAddress + BYTE_ALIGNMENT;
	/*Align the strating adress */
	descAddress &= ~0x3F;
	cppiInfo.region0DescriptorAddress = (unsigned int *)descAddress;

	HWREG(usbInstance->otgBaseAddress + CPDMA_QUEUEMGR_REGION_0)=
									(unsigned int)cppiInfo.region0DescriptorAddress;
	/*queue manager cotrol setup */
	HWREG(usbInstance->otgBaseAddress + CPDMA_QUEUEMGR_REGION_0_CONTROL) =
										QUEUE_MGR_DESCSIZE | QUEUE_MGR_REGSIZE;

	/*initialize the  BD pool */
	Cppi41DmaInitBdPool();

	/*configure the RX channel for each end point */
	for(i=0;i<numOfEndPoimts;i++)
	{
		if((epInfo + i)->direction == CPDMA_DIR_RX)
			Cppi41DmaConfigRxhostPacket(usbDevInst, usbInstance
				->rxEndPoint[(epInfo + i)->endPoint].channel,
				usbInstance->rxEndPoint[(epInfo + i)->endPoint].submitq,
				usbInstance->rxEndPoint[(epInfo + i)->endPoint].submitq);

	}

	/*configure the RX and TX completion queues for each end point */
	for(i=0;i<numOfEndPoimts;i++)
	{
		if((epInfo + i)->direction == CPDMA_DIR_RX)
		{
		 	Cppi41DmaConfigRxCompletionQueue(usbDevInst, usbInstance
		 	->rxEndPoint[(epInfo + i)->endPoint].channel, usbInstance
		 					->rxEndPoint[(epInfo + i)->endPoint].complettionq);
		}
		else
		{
		 	Cppi41DmaConfigTxCompletionQueue(usbDevInst, usbInstance
			->txEndPoint[(epInfo + i)->endPoint].channel, usbInstance
							->txEndPoint[(epInfo + i)->endPoint].complettionq);
		}
	}

	/*Scheduler configuration */
	/*Set the oerder of each RX and TX chennel to Processed to scheduler table 0 */
	HWREG(usbInstance->otgBaseAddress + CPDMA_SCHED_TABLE_0) = SCHEDULE_TX_CHANNEL;
	HWREG(usbInstance->otgBaseAddress + CPDMA_SCHED_TABLE_1) = SCHEDULE_RX_CHANNEL;

	/*Specify the number of entries in the scheduler table 0 */
	HWREG(usbInstance->otgBaseAddress + CPDMA_SCHED_CONTROL_REG )=
								(ENABLE_CPPIDMA<< SCHEDULER_ENABLE_SHFT) |
											(NUM_OF_SCHEDULER_ENTRIES - 1);

}

/**
* \brief	  This API sets the DMA mode for each channel
*
* \param	  dmaMde :- mode for rx and Tx.
*
*\param	  epInfo:- End point info structure
*
* \return	  None.
*
**/
void Cppi41DmaModeSet(unsigned short usbDevInst, endpointInfo *epInfo)
{

	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	/* Enable RNDIS from Global Level */
	if (epInfo->dmaMode == CPDMA_MODE_SET_RNDIS)
	{
		HWREG(usbInstance->otgBaseAddress + USB_0_CTRL)|=
												CPDMA_MODE_ENABLE_GLOBAL_RNDIS;
	}

	/* Disable RNDIS from Global Level	 */
	else
	{
		HWREG(usbInstance->otgBaseAddress + USB_0_CTRL)&=
												~CPDMA_MODE_ENABLE_GLOBAL_RNDIS;
	}

	/*Set the mode for RX and TX channel */
	if(epInfo->direction == CPDMA_DIR_TX)
	{
		HWREG(usbInstance->otgBaseAddress + USB_0_MODE)|=
					(epInfo->dmaMode << USB_TX_MODE_SHIFT(epInfo->endPoint));
	}

	else
	{
		HWREG(usbInstance->otgBaseAddress + USB_0_MODE)|=
					(epInfo->dmaMode<< USB_RX_MODE_SHIFT(epInfo->endPoint));
	}

	/* For GRNDIS mode, set the maximum packet length */
	if (epInfo->dmaMode == CPDMA_MODE_SET_GRNDIS)
	{
		HWREG(usbInstance->otgBaseAddress + USB_0_GEN_RNDIS_SIZE_EP1) =
			GRNDIS_MAX_PACKET_LENGTH;
	}
}


/**
* \brief	  This API enables the TX DMA for an endpoint. The API access the CSR
*		  register of  the particular endpoint to set appropriate bits. This API is
*		  called by the stack to enable DMA when it is requied.
*
* \param	  usbDevInst:- USB device instance
*
* \param  ulEndpoint:- Endpoint Number
*
* \return	  None.
*
**/
void enableCoreTxDMA(unsigned short usbDevInst, unsigned int ulEndpoint)
{
	unsigned int ulRegister;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	/*Find out the TX CRS address */
	ulRegister = USB_O_TXCSRL1 + EP_OFFSET(ulEndpoint);

	/*Wait till completion of any previous transaction */
	while ((HWREGH(usbInstance->usbBaseAddress + ulRegister) & 0x2) == 0x02);

	/* Clear Autoset */
	HWREGH(usbInstance->usbBaseAddress + ulRegister)&= CPDMA_TX_CLR_AUTO_SET;

	/* Set DMAReqEnab & DMAReqMode */
	HWREGH(usbInstance->usbBaseAddress + ulRegister)|=CPDMA_TX_SET_REQ_ENABLE;

}


/**
* \brief	  This API enables the RX DMA for an endPoint.The API access the CSR
*		  register of  the particular endpoint to set appropriate bits. This API is
*		  called by the stack to enable DMA when it is requied.
*
* \param	  usbDevInst:- USB device instance
*
* \param  ulEndpoint:- Endpoint Number
*
* \return	  None.
*
**/
void enableCoreRxDMA(unsigned short usbDevInst, unsigned int ulEndpoint)
{
	unsigned int ulRegister;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	/*Find out the RX CSR  address */
	ulRegister = USB_O_RXCSRL1 + EP_OFFSET(ulEndpoint);

	/*Find out the index to usb intance end point array */
	ulEndpoint = USB_EP_TO_INDEX(ulEndpoint);

	/*Enable the scheduler with number entries */
	Cppi41DmaControlScheduler(usbDevInst, ENABLE_CPPIDMA, NUM_OF_SCHEDULER_ENTRIES);

	/*Configure the completion queue */
	Cppi41DmaConfigRxCompletionQueue(usbDevInst, usbInstance->rxEndPoint[ulEndpoint].channel,
		 				 			usbInstance->rxEndPoint[ulEndpoint].complettionq);

	/* Clear AUTOCLEAR and DMAReqMode */
	HWREGH(usbInstance->usbBaseAddress + ulRegister)&=CPDMA_RX_CLR_AUTO_CLEAR;

	/* Set DMAReqEnab */
	HWREGH(usbInstance->usbBaseAddress + ulRegister)|=CPDMA_RX_SET_REQ_ENABLE;
}

/**
* \brief	  This API disables the RX DMA for an endpoint. The API access the CSR
*		  register of  the particular endpoint to set appropriate bits. This API is
*		  Called by the stack to disable DMA when it is required
*
* \param	  usbDevInst:- USB device instance
*
* \param  ulEndpoint:- Endpoint Number
*
* \return	  None.
*
**/
void disableCoreRxDMA(unsigned short usbDevInst, unsigned int ulEndpoint)
{
	unsigned int ulRegister;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	/*Find out the CSR offset */
	ulRegister = USB_O_RXCSRL1 + EP_OFFSET(ulEndpoint);

	/*wait till completion of any previos transaction */
	while ((HWREGH(usbInstance->usbBaseAddress + ulRegister) & 0x1) == 0x01);

	/* Clear DMAReqEnab */
	HWREGH(usbInstance->usbBaseAddress + ulRegister)&= CPDMA_RX_CLR_REQ_ENABLE;
}

/**
* \brief	  This API enables the TX DMA for an endpont. The API access the CSR
*		  register of  the particular endpoint to set appropriate bits. This API is
*		  Called by the stack to disable DMA when it is required
*
* \param	  usbDevInst:- USB device instance
*
* \param  ulEndpoint:- Endpoint Number
*
* \return	  None.
*
**/
void disableCoreTxDMA(unsigned short usbDevInst, unsigned int ulEndpoint)
{
	unsigned int ulRegister;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	ulRegister = USB_O_TXCSRL1 + EP_OFFSET(ulEndpoint);
	while ((HWREGH(usbInstance->usbBaseAddress + ulRegister) & 0x2) == 0x02);

	/* Clear AUTOSET */
	HWREGH(usbInstance->usbBaseAddress + ulRegister)&=CPDMA_TX_CLR_AUTO_SET;

	/* Clear DMAReqEnab & DMAReqMode */
	HWREGH(usbInstance->usbBaseAddress + ulRegister)&=CPDMA_TX_CLR_REQ_ENABLE;
}

/**
* \brief	  This API reads the complettion queue. This is called from the stack when a completion
* 		   interrupt comes. RX/TX completion queue number can be passed as an argument to
*		   this function. The function returns completed BD address.
*
* \param	  usbDevInst:- USB device instance
*
* \param  queueNum:- The completeion queue number
*
* \return	  None.
*
**/
unsigned int Cppi41DmaReadCompletionQueue(unsigned short usbDevInst,
														unsigned int queueNum)
{
	unsigned int  descAddress;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	/*Read the completetion queue register */
	descAddress=(unsigned int)HWREG(usbInstance->otgBaseAddress +
							(CPDMA_QUEUE_REGISTER_D + QUEUE_OFFSET(queueNum)));
	/*Extract the BD address  */
	descAddress&=0xFFFFFFE0;
	return(descAddress);
}

/**
* \brief	This API  configures an RX channel submit queue
*
* \param	 usbDevInst:- The USB Device instance
*
* \param	 chan:- The channel Number
*
* \param  rxSubmitqA:- The queue register A value
*
* \param	  rxSubmitqB:- The queue register B value
*
* \return	  None.
*
**/
void Cppi41DmaConfigRxhostPacket(unsigned short usbDevInst, unsigned short chan,
							unsigned int rxSubmitqA, unsigned int rxSubmitqB)
{
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	HWREG(usbInstance->otgBaseAddress + (CPDMA_RX_CHANNEL_REG_A +
											CHANNEL_OFFSET(chan))) = (rxSubmitqA | rxSubmitqA << 16);
	HWREG(usbInstance->otgBaseAddress + (CPDMA_RX_CHANNEL_REG_B +
											CHANNEL_OFFSET(chan))) = (rxSubmitqB | rxSubmitqB << 16) ;
}

/**
* \brief	 Configure the RX completion queues
*
* \param	 usbDevInst:- The USB Device instance
*
* \param	 rxChan:- The channel Number
*
* \param  rxCompq:-RX Completion queue
*
* \return	  None.
*
**/
void Cppi41DmaConfigRxCompletionQueue(unsigned short usbDevInst,
								unsigned short rxChan,unsigned int rxCompq)
{
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	HWREG(usbInstance->otgBaseAddress + (CPDMA_RX_CHANNEL_CONFIG_REG +
				CHANNEL_OFFSET(rxChan))) = CPDMA_RX_CHANNEL_ENABLE	 | rxCompq;

}


/**
* \brief	 Configure the TX completion queues
*
* \param	 usbDevInst:- The USB Device instance
*
* \param	 txChan:- The channel Number
*
* \param  txCompq:-RX Completion queue
*
* \return	  None.
*
**/
void Cppi41DmaConfigTxCompletionQueue(unsigned short usbDevInst,
									unsigned short txChan, unsigned int txCompq)
{
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	HWREG(usbInstance->otgBaseAddress + (CPDMA_TX_CHANNEL_CONFIG_REG +
				CHANNEL_OFFSET(txChan))) = 	CPDMA_TX_CHANNEL_ENABLE	 | txCompq;
}



/**
* \brief	 Initializes the BD pool
*
* \param
*
* \return	  None.
*
**/
void Cppi41DmaInitBdPool()
{
	hostPacketDesc *temp_bd = NULL;
	unsigned int i;

	temp_bd = (hostPacketDesc *)cppiInfo.region0DescriptorAddress;
	for(i=0;i<MAX_BD_NUM; i++, temp_bd++)
	{
		if(!cppiInfo.head_bd)
		{
			cppiInfo.head_bd = temp_bd;
			cppiInfo.tail_bd = temp_bd;
			temp_bd->nextHBDptr = NULL;
		}
		else
		{
			cppiInfo.tail_bd->nextHBDptr = temp_bd;
			cppiInfo.tail_bd = cppiInfo.tail_bd->nextHBDptr;
			temp_bd->nextHBDptr = NULL;
		}
	}
	temp_bd = NULL;
}


/**
* \brief	This API will get a free BD from the BD pool
*
* \param
*
* \return	  None.
*
**/
hostPacketDesc * getFreeBd()
{
	hostPacketDesc *free_bd;

	free_bd = NULL;

	if(cppiInfo.head_bd)
	{
		free_bd = cppiInfo.head_bd;
		cppiInfo.head_bd = cppiInfo.head_bd->nextHBDptr;
		free_bd->nextHBDptr = NULL;
	}

	return free_bd;
}

/**
* \brief	 This API puts the free BD back to the BD pool
*
* \param
*
* \return	  None.
*
**/
void putFreeBd(hostPacketDesc *free_bd)
{
	if(cppiInfo.head_bd)
	{
		free_bd->nextHBDptr = cppiInfo.head_bd;
		cppiInfo.head_bd =free_bd;
	}
	else
	{
		cppiInfo.head_bd = free_bd;
		cppiInfo.head_bd->nextHBDptr = NULL;
	}
}

/**
* \brief	 API to submit a BD to the queue
*
* \param	  usbDevInst:- The USB device instance
*
* \param  queueNum:- The submit queue number
*
* \param curr_bd:- Address of BD to submit
*
* \return	  None.
*
**/
void pushToSubmitQ(unsigned short usbDevInst,
							unsigned int  queueNum, hostPacketDesc *curr_bd)
{

	unsigned int submitqAdd;
	unsigned int curr_bdAdd;
	hostPacketDesc *bdAddr;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	/*get the BD address */
	curr_bdAdd = (unsigned int) curr_bd;
	/* Add the length to it */
	curr_bdAdd |= SIZE_OF_SINGLE_BD;
	bdAddr = (hostPacketDesc *)curr_bdAdd;
	/*Get the submit queue address */
	submitqAdd = usbInstance->otgBaseAddress + (CPDMA_QUEUE_REGISTER_D +
														QUEUE_OFFSET(queueNum));
	/*submit the queue */
	HWREG(submitqAdd) = (unsigned int) bdAddr;

}

/**
* \brief	 This API gets a free BD , process it for TX transfer and submit it to the TX queue.
*		 The API can take care of packetizing the data according to USB transfer size and
*		 attach to the BDs. This API will work in both transparent mode and GRNDIS mode .
*
* \param	  lenght:- length of the data buffer
*
* \param	  buff :- Bata buffer
*
* \param	  endPoind :- The TX end point
*
* \return	  None.
*
**/
#ifdef _TMS320C6X
void doDmaTxTransfer(unsigned short usbDevInst, unsigned char *buff,
								unsigned int length, unsigned int endPoint)
{
	hostPacketDesc *current_bd = NULL;

	unsigned int numOfBlocks =0;
	unsigned int residue;
	unsigned int i;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	endPoint = USB_EP_TO_INDEX(endPoint);

	/*This code segment will take care of the transparent mode transaction */
	if(CPDMA_MODE_SET_TRANSPARENT == usbInstance->txEndPoint[endPoint].mode)
	{
		/* If the data length is less than the USB packet size */
		if(length < USB_PACKET_LENGTH)
		{
			/*get a free db from the BD pool */
			current_bd = getFreeBd();

			/*Clean the cache so that buffer will have proper data */
			CacheWB((unsigned int)buff, length);

			/*This API will initialize the BD fields */
			Cppi41DmaProcessBD(usbDevInst, current_bd, CPDMA_DIR_TX, buff, length, endPoint);

			current_bd->packetId = EOP;

			/*Clean the cache so that bd  will have proper data */
			CacheWB((unsigned int)current_bd,sizeof(hostPacketDesc));

			/*Submit the BD to the queue for transaction */
			pushToSubmitQ(usbDevInst, usbInstance->txEndPoint[endPoint]
				.submitq, current_bd);
		}

		/*If the length is more than packet size, then packetize it */
		else
		{
			numOfBlocks = length / USB_PACKET_LENGTH;
			residue = length -  (numOfBlocks * USB_PACKET_LENGTH);

			for(i=0; i < numOfBlocks; i++)
			{
				/*get a free db from the BD pool */
				current_bd = getFreeBd();

				CacheWB((unsigned int)&buff[i*USB_PACKET_LENGTH],
					USB_PACKET_LENGTH);

				/*This API will initialize the BD fields */
				Cppi41DmaProcessBD(usbDevInst, current_bd, CPDMA_DIR_TX,
									&buff[i*USB_PACKET_LENGTH], USB_PACKET_LENGTH, endPoint);

				/*Set the packet id to identify the packet position */
				if(numOfBlocks == 1)
					current_bd->packetId = EOP;
				else if(i==0)
					current_bd->packetId = SOP;
				else if((numOfBlocks != 1) && (i < (numOfBlocks -1)))
					current_bd->packetId = MOP;
				else if(i == (numOfBlocks -1) && residue == 0)
					current_bd->packetId = EOP;
				else
					current_bd->packetId = MOP;

				CacheWB((unsigned int)current_bd,sizeof(hostPacketDesc));

				/*Submit the BD to the queue for transaction */
				pushToSubmitQ(usbDevInst, usbInstance->txEndPoint[endPoint]
					.submitq, current_bd);
			}

			/*If there are some remaining data then send it also */
			if(residue)
			{
				/*get a free db from the BD pool */
				current_bd = getFreeBd();

				CacheWB((unsigned int)&buff[i*USB_PACKET_LENGTH], residue);

				/*This API will initialize the BD fields */
				Cppi41DmaProcessBD(usbDevInst, current_bd, CPDMA_DIR_TX,
									&buff[i*USB_PACKET_LENGTH], residue, endPoint);

				current_bd->packetId = EOP;

				/*This API will initialize the BD fields */
				CacheWB((unsigned int)current_bd,sizeof(hostPacketDesc));

				/*Submit the BD to the queue for transaction */
				pushToSubmitQ(usbDevInst, usbInstance->txEndPoint[endPoint]
					.submitq, current_bd);
			}

		}
	}

	/*This segment of the code will take care of GRNIS mode of transaction */
	else if(usbInstance->txEndPoint[endPoint].mode == CPDMA_MODE_SET_GRNDIS)
	{
		/*get a free db from the BD pool */
		current_bd = getFreeBd();

		CacheWB((unsigned int)buff, length);

		/*This API will initialize the BD fields */
		Cppi41DmaProcessBD(usbDevInst, current_bd, CPDMA_DIR_TX, buff, length, endPoint);

		current_bd->packetId = EOP;

		CacheWB((unsigned int)current_bd,sizeof(hostPacketDesc));

		/*Submit the BD to the queue for transaction */
		pushToSubmitQ(usbDevInst, usbInstance->txEndPoint[endPoint]
			.submitq, current_bd);
	}

}
#else
void doDmaTxTransfer(unsigned short usbDevInst, unsigned char *buff,
								unsigned int length, unsigned int endPoint)
{
	hostPacketDesc *current_bd = NULL;

	unsigned int numOfBlocks =0;
	unsigned int residue;
	unsigned int i;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	endPoint = USB_EP_TO_INDEX(endPoint);

	/*This code segment will take care of the transparent mode transaction */
	if(CPDMA_MODE_SET_TRANSPARENT == usbInstance->txEndPoint[endPoint].mode)
	{
		/* If the data length is less than the USB packet size */
		if(length < USB_PACKET_LENGTH)
		{
			/*get a free db from the BD pool	 */
			current_bd = getFreeBd();
			ASSERT(current_bd != NULL);

			/*Clean the cache so that buffer will have proper data */
			CP15DCacheCleanBuff((unsigned int)buff, length);

			/*This API will initialize the BD fields */
			Cppi41DmaProcessBD(usbDevInst, current_bd, CPDMA_DIR_TX, buff, length, endPoint);

			current_bd->packetId = EOP;

			/*Clean the cache so that bd  will have proper data */
			CP15DCacheCleanBuff((unsigned int)current_bd,sizeof(hostPacketDesc));

			/*Submit the BD to the queue for transaction	 */
			pushToSubmitQ(usbDevInst, usbInstance->txEndPoint[endPoint]
				.submitq, current_bd);
		}

		/*If the length is more than packet size, then packetize it  */
		else
		{
			numOfBlocks = length / USB_PACKET_LENGTH;
			residue = length -  (numOfBlocks * USB_PACKET_LENGTH);

			for(i=0; i < numOfBlocks; i++)
			{
				/*get a free db from the BD pool	 */
				current_bd = getFreeBd();
				ASSERT(current_bd != NULL);

				CP15DCacheCleanBuff((unsigned int)&buff[i*USB_PACKET_LENGTH],
					USB_PACKET_LENGTH);

				/*This API will initialize the BD fields */
				Cppi41DmaProcessBD(usbDevInst, current_bd, CPDMA_DIR_TX,
									&buff[i*USB_PACKET_LENGTH], USB_PACKET_LENGTH, endPoint);

				/*Set the packet id to identify the packet position */
				if(numOfBlocks == 1)
					current_bd->packetId = EOP;
				else if(i==0)
					current_bd->packetId = SOP;
				else if((numOfBlocks != 1) && (i < (numOfBlocks -1)))
					current_bd->packetId = MOP;
				else if(i == (numOfBlocks -1) && residue == 0)
					current_bd->packetId = EOP;
				else
					current_bd->packetId = MOP;

				CP15DCacheCleanBuff((unsigned int)current_bd,sizeof(hostPacketDesc));

				/*Submit the BD to the queue for transaction	 */
				pushToSubmitQ(usbDevInst, usbInstance->txEndPoint[endPoint]
					.submitq, current_bd);
			}

			/*If there are some remaining data then send it also */
			if(residue)
			{
				/*get a free db from the BD pool	 */
				current_bd = getFreeBd();
				ASSERT(current_bd != NULL);

				CP15DCacheCleanBuff((unsigned int)&buff[i*USB_PACKET_LENGTH], residue);

				/*This API will initialize the BD fields */
				Cppi41DmaProcessBD(usbDevInst, current_bd, CPDMA_DIR_TX,
									&buff[i*USB_PACKET_LENGTH], residue, endPoint);

				current_bd->packetId = EOP;

				/*This API will initialize the BD fields */
				CP15DCacheCleanBuff((unsigned int)current_bd,sizeof(hostPacketDesc));

				/*Submit the BD to the queue for transaction	 */
				pushToSubmitQ(usbDevInst, usbInstance->txEndPoint[endPoint]
					.submitq, current_bd);
			}

		}
	}

	/*This segment of the code will take care of GRNIS mode of transaction */
	else if(usbInstance->txEndPoint[endPoint].mode == CPDMA_MODE_SET_GRNDIS)
	{
		/*get a free db from the BD pool	 */
		current_bd = getFreeBd();
		ASSERT(current_bd != NULL);

		CP15DCacheCleanBuff((unsigned int)buff, length);

		/*This API will initialize the BD fields */
		Cppi41DmaProcessBD(usbDevInst, current_bd, CPDMA_DIR_TX, buff, length, endPoint);

		current_bd->packetId = EOP;

		CP15DCacheCleanBuff((unsigned int)current_bd,sizeof(hostPacketDesc));

		/*Submit the BD to the queue for transaction	 */
		pushToSubmitQ(usbDevInst, usbInstance->txEndPoint[endPoint]
			.submitq, current_bd);
	}

}
#endif

/**
* \brief	 This API gets a free BD , process it for RX transfer and submit it to the RX queue.
*		 This API will work in both transparent mode and GRNDIS mode. . Max transfer size
*		 of this rx transfer is max USB packet size
*
* \param	  lenght:- length of the data buffer
*
* \param	  buff :- Bata buffer
*
* \param	  endPoind :- The RX end point
*
* \return	  None.
*
**/
#ifdef _TMS320C6X
void doDmaRxTransfer(unsigned short usbDevInst, unsigned int length,
								unsigned char *buff, unsigned int endPoint)
{
	hostPacketDesc *current_bd = NULL;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	current_bd = getFreeBd();
	endPoint = USB_EP_TO_INDEX(endPoint);

	/* Process the BD */
	Cppi41DmaProcessBD(usbDevInst, current_bd, CPDMA_DIR_RX, buff, length, endPoint);

	/* Clean the cache */
	CacheWB((unsigned int)current_bd,sizeof(hostPacketDesc));

	/*Submit the BD */
	pushToSubmitQ(usbDevInst, usbInstance->rxEndPoint[endPoint].submitq, current_bd);

}
#else
void doDmaRxTransfer(unsigned short usbDevInst, unsigned int length,
								unsigned char *buff, unsigned int endPoint)
{
	hostPacketDesc *current_bd = NULL;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	current_bd = getFreeBd();
	ASSERT(current_bd != NULL);

	endPoint = USB_EP_TO_INDEX(endPoint);

	/* Process the BD */
	Cppi41DmaProcessBD(usbDevInst, current_bd, CPDMA_DIR_RX, buff, length, endPoint);

	/* Clean the cache */
	CP15DCacheCleanBuff((unsigned int)current_bd,sizeof(hostPacketDesc));

	/*Submit the BD */
	pushToSubmitQ(usbDevInst, usbInstance->rxEndPoint[endPoint].submitq, current_bd);

}
#endif
/**
* \brief	 This API reads the TX completion queue and puts the free bd to the BD pool.
*		 Extracts the buffer address from the BD and frees it. This will return the
*		 current DMA status on TX operation to the caller. his API will be called on
*		reception of a completion interrupt
*
* \param	 usbDevIns:- The USB device instacnce
*
* \param	 ulEndpoint :- Endpoint Number*
*
* \return	  None.
*
**/
#ifdef _TMS320C6X
unsigned int dmaTxCompletion(unsigned short usbDevInst, unsigned int ulEndpoint )
{

	hostPacketDesc *completed_bd;
	unsigned int ulRegister;
	unsigned int state;
	unsigned int packetid;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	ulRegister = USB_O_TXCSRL1 + EP_OFFSET( ulEndpoint);

	ulEndpoint = USB_EP_TO_INDEX(ulEndpoint);


	/*read the compltetion queue */
	completed_bd = (hostPacketDesc *)Cppi41DmaReadCompletionQueue(usbDevInst, usbInstance
											->txEndPoint[ulEndpoint].complettionq);
	/*Get the packet ID to update the DMA status */
	packetid = completed_bd->packetId;
	if(packetid == EOP)
		state = DMA_TX_COMPLETED;
	else
		state = DMA_TX_IN_PROGRESS;

	/*wait till Tx completion */
	if(state == DMA_TX_COMPLETED)
		while ((HWREGH(usbInstance->usbBaseAddress + ulRegister) & 0x2) == 0x02);

	CacheInv((unsigned int)completed_bd->buffAdd, sizeof(completed_bd->buffAdd));

	cppiDmaFreenBuffer((unsigned int *)completed_bd->buffAdd);

	completed_bd->buffAdd = 0;

	/*put the free buffer */
	putFreeBd(completed_bd);

	return state;

}
#else
unsigned int dmaTxCompletion(unsigned short usbDevInst, unsigned int ulEndpoint )
{

	hostPacketDesc *completed_bd;
	unsigned int ulRegister;
	unsigned int state;
	unsigned int packetid;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	ulRegister = USB_O_TXCSRL1 + EP_OFFSET( ulEndpoint);

	ulEndpoint = USB_EP_TO_INDEX(ulEndpoint);


	/*read the compltetion queue */
	completed_bd = (hostPacketDesc *)Cppi41DmaReadCompletionQueue(usbDevInst, usbInstance
											->txEndPoint[ulEndpoint].complettionq);
	/*Get the packet ID to update the DMA status */
	packetid = completed_bd->packetId;
	if(packetid == EOP)
		state = DMA_TX_COMPLETED;
	else
		state = DMA_TX_IN_PROGRESS;

	/*wait till Tx completion */
	if(state == DMA_TX_COMPLETED)
		while ((HWREGH(usbInstance->usbBaseAddress + ulRegister) & 0x2) == 0x02);

	CP15ICacheFlushBuff((unsigned int)completed_bd->buffAdd, sizeof(completed_bd->buffAdd));

	cppiDmaFreenBuffer((unsigned int *)completed_bd->buffAdd);

	completed_bd->buffAdd = 0;

	/*put the free buffer */
	putFreeBd(completed_bd);

	return state;

}
#endif
/**
* \brief	 This API reads the RX completion queue and puts the free bd to the BD pool
*		Extracts the data buffer from the DB and pass it to the caller . Extracts the
*		data buffer from the DB and pass it to the caller . This API will be called on
*		reception of a completion interrupt
*
* \param	  usbDevInst:- The USB Intance  Number
*
* \param	 ulEndpoin:- End point Number
*
* \return	  Free BD.
*
**/
#ifdef _TMS320C6X
unsigned int dmaRxCompletion(unsigned short usbDevInst, unsigned int ulEndpoint )
{
	unsigned int bufferAdd;
	unsigned int length;
	usbInstance *usbInstance;

	hostPacketDesc *rx_bd =NULL;;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	ulEndpoint = USB_EP_TO_INDEX(ulEndpoint);

	/*read the compltetion queue */
	rx_bd = (hostPacketDesc *)Cppi41DmaReadCompletionQueue(usbDevInst, usbInstance
												->rxEndPoint[ulEndpoint].complettionq);

	/*Fush the cache to update the BD */
	CacheInv((unsigned int)rx_bd, sizeof(hostPacketDesc));

	bufferAdd = rx_bd->buffAdd;
	length = rx_bd->buffLength;

	/*Flush the cache to update the buffer */
	CacheInv(bufferAdd, length);

	putFreeBd(rx_bd);

	return bufferAdd;
}
#else
unsigned int dmaRxCompletion(unsigned short usbDevInst, unsigned int ulEndpoint )
{

	unsigned int bufferAdd;
	unsigned int length;
	usbInstance *usbInstance;

    hostPacketDesc *rx_bd =NULL;;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	ulEndpoint = USB_EP_TO_INDEX(ulEndpoint);

	/*read the compltetion queue */
	rx_bd = (hostPacketDesc *)Cppi41DmaReadCompletionQueue(usbDevInst, usbInstance
												->rxEndPoint[ulEndpoint].complettionq);

	/*Fush the cache to update the BD */
	CP15ICacheFlushBuff((unsigned int)rx_bd, sizeof(hostPacketDesc));

	bufferAdd = rx_bd->buffAdd;
	length = rx_bd->buffLength;

	/*Flush the cache to update the buffer */
	CP15ICacheFlushBuff(bufferAdd, length);

	putFreeBd(rx_bd);

	return bufferAdd;
}
#endif

/**
* \brief	 This API gets the transfer completion interupt status. This API will be on
*		 reception of a USB interrupt to check the cause of interrupt.
*
* \param	 usbDevInst:- The USB Intance  Number
*
* \return	  None.
*
**/
unsigned int CppiDmaGetPendStatus(unsigned short usbDevInst)
{

#if defined (am335x_15x15) || defined(am335x)
	unsigned int pendReg2 = 0;
	unsigned int pendReg3 = 0;
#endif

	unsigned int pendReg0 = 0;

#if defined (am335x_15x15) || defined(am335x)
	pendReg2 = HWREG(USB_OTGBASE + CPDMA_PEND_2_REGISTER);
	pendReg3 = HWREG(USB_OTGBASE + CPDMA_PEND_3_REGISTER);

	pendReg2 = pendReg2 & CPDMA_TX_PENDING;
	pendReg3 = pendReg3 & CPDMA_RX_PENDING;

	pendReg0 = pendReg2 | pendReg3;

#else
	/*Read the pend register */
	pendReg0 = HWREG(USB_OTGBASE + CPDMA_PEND_0_REGISTER);
#endif
	return pendReg0;
}

/**
* \brief	 This API gets the starvation interupt status. This API will be on
*		 reception of a USB interrupt to check the cause of interrupt.
*		 This API will also disable the scheduler and clear the starvation
*		 interrupt status in order to avoid over flooding of interrupt
*
* \param	 usbDevInst:- The USB Intance  Number
*
* \return	  None.
*
**/
unsigned int CppiDmaGetINTD0Status(unsigned short usbDevInst)
{

	unsigned int intdReg0;
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	intdReg0 = HWREG(usbInstance->otgBaseAddress + CPDMA_INTD_0_REGISTER);

	if(intdReg0 == CPDMA_STAR_0_PEND ||intdReg0 == CPDMA_STAR_1_PEND)
	{
		Cppi41DmaControlScheduler(usbDevInst, ENABLE_CPPIDMA,
										(NUM_OF_SCHEDULER_ENTRIES / 2));
		Cppi41DmaClearINTD0Status(usbDevInst);
	}

	return intdReg0;
}

/**
* \brief	 This API clears the starvation interupt status
*
* \param	 usbDevInst:- The USB Intance  Number
*
* \return	  None.
*
**/
void Cppi41DmaClearINTD0Status(unsigned short usbDevInst)
{
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	HWREG(usbInstance->otgBaseAddress + CPDMA_CLEAR_INTD_0_STATUS) = CLAER_INTDO_STATUS;

}

/**
* \brief	 This API configures the scheduler
*
* \param	 Configuration entries for the channels
*
* \return	  None.
*
**/
void Cppi41DmaControlScheduler(unsigned short usbDevInst, unsigned short enableDisable,
												unsigned short numOfentries)
{
	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	HWREG(usbInstance->otgBaseAddress + CPDMA_SCHED_CONTROL_REG ) =(enableDisable
								<< SCHEDULER_ENABLE_SHFT) | (numOfentries - 1);

}

/**
* \brief	 This API to Process the BD
*
* \param	 current_bd :- the current BD Address
*
* \param	 direction:- The Transfer Direction
*
* \param	 buffadd:- The buffer address
*
* \param	 length:- Length of the buffer
*
* \return	  None.
*
**/
void Cppi41DmaProcessBD(unsigned short usbDevInst, hostPacketDesc *current_bd,
								 unsigned short direction, unsigned char * buffadd,
									   unsigned int length, unsigned int ulEndpoint)
{

	usbInstance *usbInstance;

	usbInstance = &(cppiInfo.usbInst[usbDevInst]);

	/*These values are fixed for a PD */
	current_bd->hPDword0.hostPktType = CPDMA_BD_PACKET_TYPE;
	current_bd->hPDword0.protSize=0;

	/* For TX BD, need to mention the lenght, for RX CPU will fill the feild */
	if(direction == CPDMA_DIR_TX)
		current_bd->hPDword0.pktLength= length;
	else
		current_bd->hPDword0.pktLength= 0;

	/* These vlaues are always initialized to zero */
	current_bd->hPDword1.dstTag=0;
	current_bd->hPDword1.srcSubChNum=0;
	current_bd->hPDword1.srcChNum=0;

	/*The channel Number */
	if(direction == CPDMA_DIR_TX)
		current_bd->hPDword1.srcPrtNum = usbInstance->txEndPoint[ulEndpoint]
																		.channel + 1; /*chan_num+1; */
	else
		current_bd->hPDword1.srcPrtNum = usbInstance->rxEndPoint[ulEndpoint]
																		.channel + 1; /*chan_num+1; */

	/*Completion Queues */
	if(direction == CPDMA_DIR_TX)
		current_bd->hPDword2.pktRetQueue=usbInstance->txEndPoint[ulEndpoint]
																		.complettionq; /*returnQueue; */
	else
		current_bd->hPDword2.pktRetQueue=usbInstance->rxEndPoint[ulEndpoint]
																		.complettionq; /*returnQueue; */

	current_bd->hPDword2.pktRetQM=0;
	/*The BD is on chip */
	current_bd->hPDword2.onChip=1;
	current_bd->hPDword2.retPolicy=0;
	current_bd->hPDword2.protoSpecific=0;
	current_bd->hPDword2.rsv=0;
	/*USB type packet */
	current_bd->hPDword2.pktType=5;
	current_bd->hPDword2.pktErr=0;
	current_bd->buffLength=length;
	current_bd->buffAdd=(unsigned int)buffadd;
	current_bd->nextHBDptr=0;
	current_bd->gBuffLength=current_bd->buffLength;
	current_bd->gBuffLength |= (unsigned int)(1<<31);
	current_bd->gBuffAdd=current_bd->buffAdd;
	current_bd->endPoint = (unsigned short)ulEndpoint;
	if(direction == CPDMA_DIR_TX)
		current_bd->channel = usbInstance->txEndPoint[ulEndpoint].channel;
	else
		current_bd->channel = usbInstance->rxEndPoint[ulEndpoint].channel;
	current_bd->devInst = (unsigned char)usbDevInst;

}


/**
* \brief	 This API initialize the bbuffer
*
* \param	 none
*
* \return	  None.
*
**/
void cppi41DmaInitBuffer()
{
	int i;

	for(i =0;i<CPDMA_NUMOF_BUFFERS;i++)
		{
			 dmaBuffer[i].dataBuffer =(unsigned int *)&dBuffer[i][0];
			 dmaBuffer[i].usedFlag = CPDMA_BUFFER_NOT_USED;
			 dmaBuffer[i].nBlocks = 0;
		}

}


/**
* \brief	 This API will allocate 1unit(512 bytes) of  buffer. This API is called
*		just before the RX and TX transfer. The API returns a pointer to the
*		buffer allocated , which can used by the caller for data transfer.
*
* \param	 none
*
* \return	  None.
*
**/
unsigned int * cppiDmaAllocBuffer()
{
	int i;

	for(i =0;i<CPDMA_NUMOF_BUFFERS;i++)
	{
		if(dmaBuffer[i].usedFlag ==  CPDMA_BUFFER_NOT_USED)
		{
			dmaBuffer[i].usedFlag =  CPDMA_BUFFER_USED;
			dmaBuffer[i].nBlocks = 1;
			return dmaBuffer[i].dataBuffer;
		}
	}

	return NULL;
}


/**
* \brief	 This API will free 1 unit of buffer. Caller needs to pass the buffer
*		address to be freed. Once buffer is freed, the usage flag will set
*		as BUFFER _NOT_USED
*
* \param	 dataBuffer:- Buffer address
*
* \return	  None.
*
**/
void cppiDmaFreeBuffer(unsigned int *dataBuffer)
{
	int i;

	for(i=0;i<CPDMA_NUMOF_BUFFERS;i++)
	{
		if(dmaBuffer[i].dataBuffer == dataBuffer)
		{
			dmaBuffer[i].usedFlag = CPDMA_BUFFER_NOT_USED;
			dmaBuffer[i].nBlocks = 0;
			break;
		}
	}

}

/**
* \brief	 This API will allocate N (N*512 bytes) number of buffers. This API is called
*		just before the RX and TX transfer. The API returns a pointer to the
*		buffer allocated , which can used by the caller for data transfer.
*
* \param	 numOfBlocks:- Number of buffers
*
* \return	  None.
*
**/
unsigned int * cppiDmaAllocnBuffer(unsigned int numOfBlocks)
{
	unsigned int nBlocks = numOfBlocks;
	unsigned int i,j;



	for(i =0;i<CPDMA_NUMOF_BUFFERS;i++)
	{
		if(dmaBuffer[i].usedFlag ==  CPDMA_BUFFER_NOT_USED)
			nBlocks--;
		else
			nBlocks = numOfBlocks;

		if(nBlocks == 0)
			break;

	}


	if(nBlocks == 0)
	{
		for(j = i; j>(i-numOfBlocks); j--)
		{
			dmaBuffer[j].usedFlag =  CPDMA_BUFFER_USED;
		}
		dmaBuffer[j+1].nBlocks = numOfBlocks;
		return dmaBuffer[j+1].dataBuffer;
	}
	else
		return NULL;

}


/**
* \brief	 This API will free N(N *512 Bytes) buffers. Caller needs to pass the buffer
*		address to be freed. Once buffer is freed, the usage flag will set
*		as BUFFER _NOT_USED.
*
* \param	 dataBuffer:- Buffer address
*
* \return	  None.
*
**/

void cppiDmaFreenBuffer(unsigned int *dataBuffer)
{
	unsigned int i,j;
	unsigned int nBlocks = 0;

	for(i=0;i<CPDMA_NUMOF_BUFFERS;i++)
	{
		if(dmaBuffer[i].dataBuffer == dataBuffer)
		{
			nBlocks= dmaBuffer[i].nBlocks;
			break;
		}
	}

	if(nBlocks)
	{
		for(j =i; j<(i+nBlocks); j++)
		{
			dmaBuffer[j].usedFlag = CPDMA_BUFFER_NOT_USED;
			dmaBuffer[i].nBlocks= 0;
		}

	}

}

/**
* \brief	 This API will Handle the starvation Error.  The API will enable the
*		 scheduler which is disabled due to the starvation interrupt.
*		 This will help the DMA to resume the operation after clearing the starvation
*		 interrupt.
*
* \param	 dusbDevInst:- USB device instance.
*
* \return	  None.
*
**/

void cppiDmaHandleError(unsigned int usbDevInst)
{
	Cppi41DmaControlScheduler(usbDevInst, ENABLE_CPPIDMA, NUM_OF_SCHEDULER_ENTRIES);

}


