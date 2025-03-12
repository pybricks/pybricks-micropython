/**
 * \file  vpif.c
 *
 * \brief This file contains the device abstraction layer APIs for VPIF.
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
#include "hw_vpif.h"
#include "soc_OMAPL138.h"

/* Driver APIs */
#include "vpif.h"


/*******************************************************************************
*                       INTERNAL API DEFINITIONS
*******************************************************************************/
/**
* \brief  This function enables a specific channel interrupt or error interrupt.
*         To activate an interrupt after it is enabled, use the VPIFInterruptEnableSet
*          API.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  intr       is the interrupt to be enabled.
*
* \return none.
**/
void VPIFInterruptEnable(unsigned int baseAddr, unsigned int intr)
{
    HWREG(baseAddr + INTEN) |= intr;
}
/**
* \brief  This function disables a specific channel interrupt or error interrupt.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  intr       is the interrupt to be disabled.
*
* \return none.
**/
void VPIFInterruptDisable(unsigned int baseAddr, unsigned int intr)
{
    HWREG(baseAddr + INTEN) &= ~intr;
}

/**
* \brief  This function activates a specific channel interrupt or error interrupt.
*         This is only effective after the VPIFInterruptEnable API is invoked for
*          the same interrupt event.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  intr       is the interrupt to be activated.
*
* \return none.
**/
void VPIFInterruptEnableSet(unsigned int baseAddr, unsigned int intr)
{
    HWREG(baseAddr + INTSET) |= intr;
}
/**
* \brief  This function deactivates/masks a specific channel interrupt or error interrupt.
*         This is only effective after the VPIFInterruptEnable API is invoked for
*          the same interrupt event.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  intr       is the interrupt to be deactivated/masked.
*
* \return none.
**/
void VPIFInterruptEnableClear(unsigned int baseAddr, unsigned int intr)
{
    HWREG(baseAddr + INTCLR) |= intr;
}

/**
* \brief  This function clears the interrupt status of a given interrupt inside VPIF.
*         The interrupt status at the CPU is not clearred.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  intr       is the interrupt of interest.
*
* \return none.
**/
void VPIFInterruptStatusClear(unsigned int baseAddr, unsigned int intr)
{
    HWREG(baseAddr + INTSTATCLR) |= intr;
}

/**
* \brief  This function returns whether the interrupt of interest has happened or not.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  intr       is the interrupt of interest.
*
* \return Requested interrupt status.
**/
unsigned int VPIFInterruptStatus(unsigned int baseAddr, unsigned int intr)
{
    return HWREG(baseAddr + INTSTAT) & intr;
}

/**
* \brief  This function clears the error status of a given error.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  err        is the err of interest.
*
* \return none.
**/
void VPIFErrorStatusClear(unsigned int baseAddr, unsigned int err)
{
    HWREG(baseAddr + ERRSTAT) |= err;
}

/**
* \brief  This function returns whether the error of interest has happened or not.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  err        is the error of interest.
*
* \return Requested error status.
**/
unsigned int VPIFErrorStatus(unsigned int baseAddr, unsigned int err)
{
    return HWREG(baseAddr + ERRSTAT) & err;
}

/**
* \brief  This function configures the VPIF DMA transfer size.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  size       is the transfer size.
*
* \return none.
**/
void VPIFDMARequestSizeConfig(unsigned int baseAddr, unsigned int size)
{
    HWREG(baseAddr + REQSIZE) = size;
}
/**
* \brief  This function configures the mode of opeartion during emulation halt.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  mode       is the mode of operation.
*
* \return none.
**/
void VPIFEmulationControlSet(unsigned int baseAddr, unsigned int mode)
{
    HWREG(baseAddr + EMUCTRL) = mode;
}

//C0CTRL & C1CTRL
/**
* \brief  This function selects the edge of the pixel clock that data is captured on.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
* \param  mode         is the edge of the pixel clock to be selected.
*
* \return none.
**/
void VPIFCaptureClkedgeModeSelect(unsigned int baseAddr, unsigned int channel, unsigned int mode)
{
    unsigned int temp;
    if(channel==VPIF_CHANNEL_0)
    {
        temp = HWREG(baseAddr + C0CTRL) & ~VPIF_C0CTRL_CLKEDGE;
        HWREG(baseAddr + C0CTRL) = temp | mode;
    }
    else if(channel==VPIF_CHANNEL_1)
    {
        temp = HWREG(baseAddr + C1CTRL) & ~VPIF_C1CTRL_CLKEDGE;
        HWREG(baseAddr + C1CTRL) = temp | mode;
    }
}
/**
* \brief  This function configures bit per pixel during raw capture mode.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  width      is the number of bits per pixel.
*
* \return none.
**/
void VPIFCaptureRawDatawidthConfig(unsigned int baseAddr, unsigned int width)
{
    unsigned int temp;
    temp = HWREG(baseAddr + C0CTRL) & ~VPIF_C0CTRL_DATAWIDTH;
    HWREG(baseAddr + C0CTRL) = temp | width;
}
/**
* \brief  This function configures how often interrupts are generated during raw
*          capture mode. An interrupt is generated every 'interval' number of lines.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  interval   is the number of lines.
*
* \return none.
**/
void VPIFCaptureRawIntlineConfig(unsigned int baseAddr, unsigned int interval)
{
    unsigned int temp;
    /* The number of lines should be smaller than the max supported lines in a frame */
    if(interval <= (VPIF_C0CTRL_INTLINE >> VPIF_C0CTRL_INTLINE_SHIFT))
    {
        temp = HWREG(baseAddr + C0CTRL) & ~VPIF_C0CTRL_INTLINE;
        HWREG(baseAddr + C0CTRL) = temp | (interval << VPIF_C0CTRL_INTLINE_SHIFT);
    }
}
/**
* \brief  This function sets whether the polarity of the field id signal is inverted
*          during raw capture mode.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  mode        is the polarity mode (inverted or non-inverted).
*
* \return none.
**/
void VPIFCaptureRawFidinvSet(unsigned int baseAddr, unsigned int mode)
{
    unsigned int temp;
    temp = HWREG(baseAddr + C0CTRL) & ~VPIF_C0CTRL_FIDINV;
    HWREG(baseAddr + C0CTRL) = temp | mode;
}
/**
* \brief  This function sets whether the polarity of the vertical valid signal is
*          inverted during raw capture mode.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  mode        is the polarity mode (inverted or non-inverted).
*
* \return none.
**/
void VPIFCaptureRawVvinvSet(unsigned int baseAddr, unsigned int mode)
{
    unsigned int temp;
    temp = HWREG(baseAddr + C0CTRL) & ~VPIF_C0CTRL_VVINV;
    HWREG(baseAddr + C0CTRL) = temp | mode;
}
/**
* \brief  This function sets whether the polarity of the horizontal valid signal is
*          inverted during raw capture mode.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  mode        is the polarity mode (inverted or non-inverted).
*
* \return none.
**/
void VPIFCaptureRawHvinvSet(unsigned int baseAddr, unsigned int mode)
{
    unsigned int temp;
    temp = HWREG(baseAddr + C0CTRL) & ~VPIF_C0CTRL_HVINV;
    HWREG(baseAddr + C0CTRL) = temp | mode;
}
/**
* \brief  This function configures the storage mode of the incoming data.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  mode        is the storage mode (field-based or frame-based).
*
* \return none.
**/
void VPIFCaptureFieldframeModeSelect(unsigned int baseAddr, unsigned int mode)
{
    /* Both capture channels are set together */
    unsigned int temp;
    temp = HWREG(baseAddr + C0CTRL) & ~VPIF_C0CTRL_FIELDFRAME;
    HWREG(baseAddr + C0CTRL) = temp | mode;
}
/**
* \brief  This function sets the display format of the incoming video.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
* \param  mode        is the display format (interlaced or progressive).
*
* \return none.
**/
void VPIFCaptureIntrprogModeSelect(unsigned int baseAddr, unsigned int channel, unsigned int mode)
{
    unsigned int temp;
    if(channel==VPIF_CHANNEL_0)
    {
        temp = HWREG(baseAddr + C0CTRL) & ~VPIF_C0CTRL_INTRPROG;
        HWREG(baseAddr + C0CTRL) = temp | mode;
    }
    else if(channel==VPIF_CHANNEL_1)
    {
        temp = HWREG(baseAddr + C1CTRL) & ~VPIF_C1CTRL_INTRPROG;
        HWREG(baseAddr + C1CTRL) = temp | mode;
    }
}
/**
* \brief  This function enables vertical blanking capture.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
*
* \return none.
**/
void VPIFCaptureVancEnable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_0)
    {
        HWREG(baseAddr + C0CTRL) |= VPIF_C0CTRL_VANC;
    }
    else if(channel==VPIF_CHANNEL_1)
    {
        HWREG(baseAddr + C1CTRL) |= VPIF_C1CTRL_VANC;
    }
}
/**
* \brief  This function disbles vertical blanking capture.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
*
* \return none.
**/
void VPIFCaptureVancDisable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_0)
    {
        HWREG(baseAddr + C0CTRL) &= ~VPIF_C0CTRL_VANC;
    }
    else if(channel==VPIF_CHANNEL_1)
    {
        HWREG(baseAddr + C1CTRL) &= ~VPIF_C1CTRL_VANC;
    }
}
/**
* \brief  This function enables horizontal blanking capture.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
*
* \return none.
**/
void VPIFCaptureHancEnable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_0)
    {
        HWREG(baseAddr + C0CTRL) |= VPIF_C0CTRL_HANC;
    }
    else if(channel==VPIF_CHANNEL_1)
    {
        HWREG(baseAddr + C1CTRL) |= VPIF_C1CTRL_HANC;
    }
}
/**
* \brief  This function disables horizontal blanking capture.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
*
* \return none.
**/

void VPIFCaptureHancDisable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_0)
    {
        HWREG(baseAddr + C0CTRL) &= ~VPIF_C0CTRL_HANC;
    }
    else if(channel==VPIF_CHANNEL_1)
    {
        HWREG(baseAddr + C1CTRL) &= ~VPIF_C1CTRL_HANC;
    }
}
/**
* \brief  This function configures how interrupts are generated during BT
*         video capture.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
* \param  mode        is the interrupt geneation mode (top field, bottom field,
*                     or both).
*
* \return none.
**/
void VPIFCaptureIntframeConfig(unsigned int baseAddr, unsigned int channel, unsigned int mode)
{
    unsigned int temp;
    if(channel==VPIF_CHANNEL_0)
    {
        temp = HWREG(baseAddr + C0CTRL) & ~VPIF_C0CTRL_INTFRAME;
        HWREG(baseAddr + C0CTRL) = temp | mode;
    }
    else if(channel==VPIF_CHANNEL_1)
    {
        temp = HWREG(baseAddr + C1CTRL) & ~VPIF_C1CTRL_INTFRAME;
        HWREG(baseAddr + C1CTRL) = temp | mode;
    }
}
/**
* \brief  This function returns the field id of the field being captured.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
*
* \return Current field id.
**/
unsigned int VPIFCaptureFidModeRead(unsigned int baseAddr, unsigned int channel)
{
    unsigned int temp = 0;
    if(channel==VPIF_CHANNEL_0)
    {
        temp = HWREG(baseAddr + C0CTRL) & VPIF_C0CTRL_FID;
    }
    else if(channel==VPIF_CHANNEL_1)
    {
        temp = HWREG(baseAddr + C1CTRL) & VPIF_C1CTRL_FID;
    }
    return temp;
}
/**
* \brief  This function configures the input data format.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
* \param  mode        is the data format (y/c muxed or non-muxed).
*
* \return none.
**/
void VPIFCaptureYcmuxModeSelect(unsigned int baseAddr, unsigned int channel, unsigned int mode)
{
    unsigned int temp;
    if(channel==VPIF_CHANNEL_0)
    {
        temp = HWREG(baseAddr + C0CTRL) & ~VPIF_C0CTRL_YCMUX;
        HWREG(baseAddr + C0CTRL) = temp | mode;
    }
    else if(channel==VPIF_CHANNEL_1)
    {
        temp = HWREG(baseAddr + C1CTRL) & ~VPIF_C1CTRL_YCMUX;
        HWREG(baseAddr + C1CTRL) = temp | mode;
    }
}
/**
* \brief  This function configures the capture mode.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
* \param  mode        is the capture mode (raw or BT).
*
* \return none.
**/
void VPIFCaptureCapmodeModeSelect(unsigned int baseAddr, unsigned int channel, unsigned int mode)
{
    unsigned int temp;
    if(channel==VPIF_CHANNEL_0)
    {
        temp = HWREG(baseAddr + C0CTRL) & ~VPIF_C0CTRL_CAPMODE;
        HWREG(baseAddr + C0CTRL) = temp | mode;
    }
    else if(channel==VPIF_CHANNEL_1)
    {
        temp = HWREG(baseAddr + C1CTRL) & ~VPIF_C1CTRL_CAPMODE;
        HWREG(baseAddr + C1CTRL) = temp | mode;
    }

}
/**
* \brief  This function enables capture.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
*
* \return none.
**/
void VPIFCaptureChanenEnable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_0)
    {
        HWREG(baseAddr + C0CTRL) |= VPIF_C0CTRL_CHANEN;
    }
    else if(channel==VPIF_CHANNEL_1)
    {
        HWREG(baseAddr + C1CTRL) |= VPIF_C1CTRL_CHANEN;
    }
}
/**
* \brief  This function disables capture.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
*
* \return none.
**/

void VPIFCaptureChanenDisable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_0)
    {
        HWREG(baseAddr + C0CTRL) &= ~VPIF_C0CTRL_CHANEN;
    }
    else if(channel==VPIF_CHANNEL_1)
    {
        HWREG(baseAddr + C1CTRL) &= ~VPIF_C1CTRL_CHANEN;
    }
}
/**
* \brief  This function configures the buffer for the captured blanking data.
*         The buffer address (and offset when applicable) is passed to VPIF.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
* \param  field      is the field that the blanking data is for (top/bottom).
* \param  hv         is the location of the blanking data (during horizontal/vertical period).
* \param  addr         is the address of the VBI buffer in the memory.
* \param  offset     is the line offset of the buffer in the memory if it is for horizontal blanking.
*
* \return none.
**/

void VPIFCaptureVBIFBConfig(unsigned int baseAddr, unsigned int channel, unsigned field, unsigned hv, unsigned int addr, unsigned int offset)
{
    if(channel==VPIF_CHANNEL_0)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                HWREG(baseAddr + C0THANC) = addr;
                HWREG(baseAddr + C0HANCOFFSET) = offset;

            }
            if(hv==VPIF_VERTICAL)
            {
                HWREG(baseAddr + C0TVANC) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                HWREG(baseAddr + C0BHANC) = addr;
                HWREG(baseAddr + C0HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                HWREG(baseAddr + C0BVANC) = addr;
            }
        }
    }
    if(channel==VPIF_CHANNEL_1)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                HWREG(baseAddr + C1THANC) = addr;
                HWREG(baseAddr + C1HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                HWREG(baseAddr + C1TVANC) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                HWREG(baseAddr + C1BHANC) = addr;
                HWREG(baseAddr + C1HANCOFFSET) = offset;

            }
            if(hv==VPIF_VERTICAL)
            {
                HWREG(baseAddr + C1BVANC) = addr;
            }
        }
    }
}

/**
* \brief  This function exchanges the buffer for the captured blanking data.
*          A new buffer address (and offset when applicable) is passed in, and
*          the old buffer address is read back.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
* \param  field      is the field that the blanking data is for (top/bottom).
* \param  hv         is the location of the blanking data (during horizontal/vertical period).
* \param  addr         is the address of the VBI buffer in the memory.
* \param  offset     is the line offset of the buffer in the memory if it is for horizontal blanking.
*
* \return Previous VBI buffer address.
**/

unsigned int VPIFCaptureVBIFBExchange(unsigned int baseAddr, unsigned int channel, unsigned field, unsigned hv, unsigned int addr, unsigned int offset)
{
    unsigned int temp = 0;
    if(channel==VPIF_CHANNEL_0)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                temp = HWREG(baseAddr + C0THANC);
                HWREG(baseAddr + C0THANC) = addr;
                HWREG(baseAddr + C0HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                temp = HWREG(baseAddr + C0TVANC);
                HWREG(baseAddr + C0TVANC) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                temp = HWREG(baseAddr + C0BHANC);
                HWREG(baseAddr + C0BHANC) = addr;
                HWREG(baseAddr + C0HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                temp = HWREG(baseAddr + C0BVANC);
                HWREG(baseAddr + C0BVANC) = addr;
            }
        }
    }
    if(channel==VPIF_CHANNEL_1)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                temp = HWREG(baseAddr + C1THANC);
                HWREG(baseAddr + C1THANC) = addr;
                HWREG(baseAddr + C1HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                temp = HWREG(baseAddr + C1TVANC);
                HWREG(baseAddr + C1TVANC) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                temp = HWREG(baseAddr + C1BHANC);
                HWREG(baseAddr + C1BHANC) = addr;
                HWREG(baseAddr + C1HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                temp = HWREG(baseAddr + C1BVANC);
                HWREG(baseAddr + C1BVANC) = addr;
            }
        }
    }
    return temp;
}
/**
* \brief  This function configures the dimension of the video to be captured
*          (both active portion and blanking portion). The application doesn't
*         need to specify dimension information, except when non-standard BT
*         video is to be captured.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  mode       is the video standard (i.e., 480I, raw, non-standard).
* \param  sdChannel  is the capture channel (channel 1 or channel 0).
* \param  rawWidth     is the bit-per-pixel of the raw video to be captured.
* \param  *buf         is the dimension information of the non-standard video to be captured.
*                     It is of type vbufParam (structure).
*
* \return none.
**/

void VPIFCaptureModeConfig(unsigned int baseAddr, unsigned int mode, unsigned int sdChannel, unsigned int rawWidth, VPIFVbufParam* buf)
{    /*
    typedef struct vbufParam
    {
    unsigned int sav2eav;
    unsigned int eav2sav;
    unsigned int vsize;
    unsigned int l1;
    unsigned int l3;
    unsigned int l5;
    unsigned int l7;
    unsigned int l9;
    unsigned int l11;
    } VPIFVbufParam;
    */
    if(mode==VPIF_480I)
    {
        if(sdChannel==VPIF_CHANNEL_0)
        {
            HWREG(baseAddr + C0HCFG) = (268 << VPIF_C0HCFG_EAV2SAV_SHIFT) | (1440 << VPIF_C0HCFG_SAV2EAV_SHIFT);
            HWREG(baseAddr + C0VCFG0) = (4 << VPIF_C0VCFG0_L1_SHIFT) | (20 << VPIF_C0VCFG0_L3_SHIFT);
            HWREG(baseAddr + C0VCFG1) = (264 << VPIF_C0VCFG1_L5_SHIFT) | (266 << VPIF_C0VCFG1_L7_SHIFT);
            HWREG(baseAddr + C0VCFG2) = (283 << VPIF_C0VCFG2_L9_SHIFT) | (1 << VPIF_C0VCFG2_L11_SHIFT);
            HWREG(baseAddr + C0VSIZE) = 525 << VPIF_C0VSIZE_VSIZE_SHIFT;
        }
        if(sdChannel==VPIF_CHANNEL_1)
        {
            HWREG(baseAddr + C1HCFG) = (268 << VPIF_C1HCFG_EAV2SAV_SHIFT) | (1440 << VPIF_C1HCFG_SAV2EAV_SHIFT);
            HWREG(baseAddr + C1VCFG0) = (4 << VPIF_C1VCFG0_L1_SHIFT) | (20 << VPIF_C1VCFG0_L3_SHIFT);
            HWREG(baseAddr + C1VCFG1) = (264 << VPIF_C1VCFG1_L5_SHIFT) | (266 << VPIF_C1VCFG1_L7_SHIFT);
            HWREG(baseAddr + C1VCFG2) = (283 << VPIF_C1VCFG2_L9_SHIFT) | (1 << VPIF_C1VCFG2_L11_SHIFT);
            HWREG(baseAddr + C1VSIZE) = 525 << VPIF_C1VSIZE_VSIZE_SHIFT;
        }
    }
    if(mode==VPIF_CAPTURE_RAW)
    {
        /* TBD */
    }
    if(mode==VPIF_NONSTANDARD)
    {
        /* TBD */
        if(sdChannel==VPIF_CHANNEL_0);
        if(sdChannel==VPIF_CHANNEL_1);
    }
    
    /* TBD */
}

/**
* \brief  This function configures the frame buffer of the captured video.
*         The buffer address (and offset when applicable) is passed to VPIF.

*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
* \param  field      is the field that the video is for (top/bottom).
* \param  yc         is the data component (luma/chroma).
* \param  addr       is the address of the frame buffer in the memory.
* \param  offset     is the line offset of the frame buffer in the memory.
*
* \return none.
**/

void VPIFCaptureFBConfig(unsigned int baseAddr, unsigned int channel, unsigned int field, unsigned yc, unsigned int addr, unsigned int offset)
{
    if(channel==VPIF_CHANNEL_0)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                HWREG(baseAddr + C0TLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                HWREG(baseAddr + C0TCHROMA) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                HWREG(baseAddr + C0BLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                HWREG(baseAddr + C0BCHROMA) = addr;
            }
        }
        HWREG(baseAddr + C0IMGOFFSET) = offset;
    }
    if(channel==VPIF_CHANNEL_1)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                HWREG(baseAddr + C1TLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                HWREG(baseAddr + C1TCHROMA) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                HWREG(baseAddr + C1BLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                HWREG(baseAddr + C1BCHROMA) = addr;
            }
        }
        HWREG(baseAddr + C1IMGOFFSET) = offset;
    }
}
/**
* \brief  This function exchanges the frame buffer of the captured video.
*         A new buffer address (and offset when applicable) is passed to VPIF,
*         and the address of the previous buffer is read back.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
* \param  field      is the field that the video is for (top/bottom).
* \param  yc         is the data component (luma/chroma).
* \param  addr       is the address of the frame buffer in the memory.
* \param  offset     is the line offset of the frame buffer in the memory.
*
* \return Previuos buffer address.
**/
unsigned int VPIFCaptureFBExchange(unsigned int baseAddr, unsigned int channel, unsigned int field, unsigned yc, unsigned int addr, unsigned int offset)
{
    unsigned int temp = 0;
    if(channel==VPIF_CHANNEL_0)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                temp = HWREG(baseAddr + C0TLUMA);
                HWREG(baseAddr + C0TLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                temp = HWREG(baseAddr + C0TCHROMA);
                HWREG(baseAddr + C0TCHROMA) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                temp = HWREG(baseAddr + C0BLUMA);
                HWREG(baseAddr + C0BLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                temp = HWREG(baseAddr + C0BCHROMA);
                HWREG(baseAddr + C0BCHROMA) = addr;
            }
        }
        HWREG(baseAddr + C0IMGOFFSET) = offset;
    }
    if(channel==VPIF_CHANNEL_1)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                temp = HWREG(baseAddr + C1TLUMA);
                HWREG(baseAddr + C1TLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                temp = HWREG(baseAddr + C1TCHROMA);
                HWREG(baseAddr + C1TCHROMA) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                temp = HWREG(baseAddr + C1BLUMA);
                HWREG(baseAddr + C1BLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                temp = HWREG(baseAddr + C1BCHROMA);
                HWREG(baseAddr + C1BCHROMA) = addr;
            }
        }
        HWREG(baseAddr + C1IMGOFFSET) = offset;
    }
    return temp;
}

//C2CTRL & C3CTRL
/**
* \brief  This function selects the edge of the pixel clock that data is displayed on.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
* \param  mode         is the edge of the pixel clock to be selected.
*
* \return none.
**/
void VPIFDisplayClkedgeModeSelect(unsigned int baseAddr, unsigned int channel, unsigned int mode)
{
    unsigned int temp;
    if(channel==VPIF_CHANNEL_2)
    {
        temp = HWREG(baseAddr + C2CTRL) & ~VPIF_C2CTRL_CLKEDGE;
        HWREG(baseAddr + C2CTRL) = temp | mode;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        temp = HWREG(baseAddr + C3CTRL) & ~VPIF_C3CTRL_CLKEDGE;
        HWREG(baseAddr + C3CTRL) = temp | mode;
    }
}
/**
* \brief  This function enables the clipping on blanking data output.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayClipancEnable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) |= VPIF_C2CTRL_CLIPANC;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) |= VPIF_C3CTRL_CLIPANC;
    }
}
/**
* \brief  This function disables the clipping on blanking data output.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayClipancDisable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) &= ~VPIF_C2CTRL_CLIPVID;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) &= ~VPIF_C3CTRL_CLIPVID;
    }
}
/**
* \brief  This function enables the clipping on active video data output.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayClipvidEnable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) |= VPIF_C2CTRL_CLIPVID;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) |= VPIF_C3CTRL_CLIPVID;
    }
}
/**
* \brief  This function disables the clipping on active video data output.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayClipvidDisable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) &= ~VPIF_C2CTRL_CLIPVID;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) &= ~VPIF_C3CTRL_CLIPVID;
    }
}
/**
* \brief  This function configures the storage mode of the outgoing data.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  mode        is the storage mode (field-based or frame-based).
*
* \return none.
**/
void VPIFDisplayFieldframeModeSelect(unsigned int baseAddr, unsigned int mode)
{
    /* Both display channels are set together */
    unsigned int temp;
    temp = HWREG(baseAddr + C2CTRL) & ~VPIF_C2CTRL_FIELDFRAME;
    HWREG(baseAddr + C2CTRL) = temp | mode;
}
/**
* \brief  This function sets the display format of the outgoing video.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
* \param  mode        is the display format (interlaced or progressive).
*
* \return none.
**/
void VPIFDisplayIntrprogModeSelect(unsigned int baseAddr, unsigned int channel, unsigned int mode)
{
    unsigned int temp;
    if(channel==VPIF_CHANNEL_2)
    {
        temp = HWREG(baseAddr + C2CTRL) & ~VPIF_C2CTRL_INTRPROG;
        HWREG(baseAddr + C2CTRL) = temp | mode;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        temp = HWREG(baseAddr + C3CTRL) & ~VPIF_C3CTRL_INTRPROG;
        HWREG(baseAddr + C3CTRL) = temp | mode;
    }
}
/**
* \brief  This function enables the display of video pixels from memory.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayPixelEnable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) |= VPIF_C2CTRL_PIXEL;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) |= VPIF_C3CTRL_PIXEL;
    }
}
/**
* \brief  This function disables the display of video pixels from memory.
*         Blank pixels (Y=0x10, C=0x80) are displayed instead.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayPixelDisable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) &= ~VPIF_C2CTRL_PIXEL;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) &= ~VPIF_C3CTRL_PIXEL;
    }
}
/**
* \brief  This function enables vertical blanking display.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayVancEnable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) |= VPIF_C2CTRL_VANC;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) |= VPIF_C3CTRL_VANC;
    }
}
/**
* \brief  This function disables vertical blanking display.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayVancDisable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) &= ~VPIF_C2CTRL_VANC;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) &= ~VPIF_C3CTRL_VANC;
    }
}
/**
* \brief  This function enables horizontal blanking display.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayHancEnable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) |= VPIF_C2CTRL_HANC;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) |= VPIF_C3CTRL_HANC;
    }
}
/**
* \brief  This function disables horizontal blanking display.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayHancDisable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) &= ~VPIF_C2CTRL_HANC;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) &= ~VPIF_C3CTRL_HANC;
    }
}
/**
* \brief  This function configures how interrupts are generated during BT
*         video display.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
* \param  mode        is the interrupt geneation mode (top field, bottom field,
*                     or both).
*
* \return none.
**/
void VPIFDisplayIntframeConfig(unsigned int baseAddr, unsigned int channel, unsigned int mode)
{
    unsigned int temp;
    if(channel==VPIF_CHANNEL_2)
    {
        temp = HWREG(baseAddr + C2CTRL) & ~VPIF_C2CTRL_INTFRAME;
        HWREG(baseAddr + C2CTRL) = temp | mode;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        temp = HWREG(baseAddr + C3CTRL) & ~VPIF_C3CTRL_INTFRAME;
        HWREG(baseAddr + C3CTRL) = temp | mode;
    }
}
/**
* \brief  This function returns the field id of the field being displayed.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return Current field id.
**/
unsigned int VPIFDisplayFidModeRead(unsigned int baseAddr, unsigned int channel)
{
    unsigned int temp = 0;
    if(channel==VPIF_CHANNEL_2)
    {
        temp = HWREG(baseAddr + C2CTRL) & VPIF_C2CTRL_FID;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        temp = HWREG(baseAddr + C3CTRL) & VPIF_C3CTRL_FID;
    }
    return temp;
}
/**
* \brief  This function configures the output data format.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
* \param  mode        is the data format (y/c muxed or non-muxed).
*
* \return none.
**/
void VPIFDisplayYcmuxModeSelect(unsigned int baseAddr, unsigned int channel, unsigned int mode)
{
    unsigned int temp;
    if(channel==VPIF_CHANNEL_2)
    {
        temp = HWREG(baseAddr + C2CTRL) & ~VPIF_C2CTRL_YCMUX;
        HWREG(baseAddr + C2CTRL) = temp | mode;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        temp = HWREG(baseAddr + C3CTRL) & ~VPIF_C3CTRL_YCMUX;
        HWREG(baseAddr + C3CTRL) = temp | mode;
    }
}
/**
* \brief  This function enables display pixel clock output.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayClkenEnable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) |= VPIF_C2CTRL_CLKEN;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) |= VPIF_C3CTRL_CLKEN;
    }
}
/**
* \brief  This function disables display pixel clock output.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayClkenDisable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) &= ~VPIF_C2CTRL_CLKEN;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) &= ~VPIF_C3CTRL_CLKEN;
    }
}
/**
* \brief  This function enables display channel.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayChanenEnable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) |= VPIF_C2CTRL_CHANEN;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) |= VPIF_C3CTRL_CHANEN;
    }
}
/**
* \brief  This function disables display channel.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
*
* \return none.
**/
void VPIFDisplayChanenDisable(unsigned int baseAddr, unsigned int channel)
{
    if(channel==VPIF_CHANNEL_2)
    {
        HWREG(baseAddr + C2CTRL) &= ~VPIF_C2CTRL_CHANEN;
    }
    else if(channel==VPIF_CHANNEL_3)
    {
        HWREG(baseAddr + C3CTRL) &= ~VPIF_C3CTRL_CHANEN;
    }
}
/**
* \brief  This function configures the buffer for the displayed blanking data.
*         The buffer address (and offset when applicable) is passed to VPIF.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
* \param  field      is the field that the blanking data is for (top/bottom).
* \param  hv         is the location of the blanking data (during horizontal/vertical period).
* \param  addr         is the address of the VBI buffer in the memory.
* \param  offset     is the line offset of the buffer in the memory if it is for horizontal blanking.
*
* \return none.
**/
void VPIFDisplayVBIFBConfig(unsigned int baseAddr, unsigned int channel, unsigned field, unsigned hv, unsigned int addr, unsigned int offset)
{
    if(channel==VPIF_CHANNEL_2)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                HWREG(baseAddr + C2THANC) = addr;
                HWREG(baseAddr + C2HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                HWREG(baseAddr + C2TVANC) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                HWREG(baseAddr + C2BHANC) = addr;
                HWREG(baseAddr + C2HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                HWREG(baseAddr + C2BVANC) = addr;
            }
        }
    }
    if(channel==VPIF_CHANNEL_3)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                HWREG(baseAddr + C3THANC) = addr;
                HWREG(baseAddr + C3HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                HWREG(baseAddr + C3TVANC) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                HWREG(baseAddr + C3BHANC) = addr;
                HWREG(baseAddr + C3HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                HWREG(baseAddr + C3BVANC) = addr;
            }
        }
    }
}
/**
* \brief  This function exchanges the buffer for the displayed blanking data.
*          A new buffer address (and offset when applicable) is passed in, and
*          the old buffer address is read back.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the capture channel (channel 1 or channel 0).
* \param  field      is the field that the blanking data is for (top/bottom).
* \param  hv         is the location of the blanking data (during horizontal/vertical period).
* \param  addr         is the address of the VBI buffer in the memory.
* \param  offset     is the line offset of the buffer in the memory if it is for horizontal blanking.
*
* \return Previous VBI buffer address.
**/
unsigned int VPIFDisplayVBIFBExchange(unsigned int baseAddr, unsigned int channel, unsigned field, unsigned hv, unsigned int addr, unsigned int offset)
{
    unsigned int temp = 0;
    if(channel==VPIF_CHANNEL_2)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                temp = HWREG(baseAddr + C2THANC);
                HWREG(baseAddr + C2THANC) = addr;
                HWREG(baseAddr + C2HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                temp = HWREG(baseAddr + C2TVANC);
                HWREG(baseAddr + C2TVANC) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                temp = HWREG(baseAddr + C2BHANC);
                HWREG(baseAddr + C2BHANC) = addr;
                HWREG(baseAddr + C2HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                temp = HWREG(baseAddr + C2BVANC);
                HWREG(baseAddr + C2BVANC) = addr;
            }
        }
    }
    if(channel==VPIF_CHANNEL_3)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                temp = HWREG(baseAddr + C3THANC);
                HWREG(baseAddr + C3THANC) = addr;
                HWREG(baseAddr + C3HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                temp = HWREG(baseAddr + C3TVANC);
                HWREG(baseAddr + C3TVANC) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                temp = HWREG(baseAddr + C3BHANC);
                HWREG(baseAddr + C3BHANC) = addr;
                HWREG(baseAddr + C3HANCOFFSET) = offset;
            }
            if(hv==VPIF_VERTICAL)
            {
                temp = HWREG(baseAddr + C3BVANC);
                HWREG(baseAddr + C3BVANC) = addr;
            }
        }
    }
    return temp;
}
/**
* \brief  This function configures the dimension/location of the displayed blanking data.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
* \param  field      is the field that the blanking data is for (top/bottom).
* \param  hv         is the location of the blanking data (during horizontal/vertical period).
* \param  vpos         is the vertical position of the blanking data in the frame.
* \param  hpos         is the horizontal position of the blanking data in the frame.
* \param  vsize         is the vertical size of the blanking data in the frame.
* \param  hsize         is the horizontal size of the blanking data in the frame.
*
* \return none.
**/
void VPIFDisplayVBIFBSizeConfig(unsigned int baseAddr, unsigned int channel, unsigned field, unsigned hv, unsigned int vpos, unsigned int hpos, unsigned int vsize, unsigned int hsize)
{
    if(channel==VPIF_CHANNEL_2)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                HWREG(baseAddr + C2THANCPOS) = (vpos << VPIF_C2THANCPOS_VPOS_SHIFT) + (hpos << VPIF_C2THANCPOS_HPOS_SHIFT);
                HWREG(baseAddr + C2THANCSIZE) = (vsize << VPIF_C2THANCSIZE_VSIZE_SHIFT) + (hsize << VPIF_C2THANCSIZE_HSIZE_SHIFT);
            }
            if(hv==VPIF_VERTICAL)
            {
                HWREG(baseAddr + C2TVANCPOS) = (vpos << VPIF_C2TVANCPOS_VPOS_SHIFT) + (hpos << VPIF_C2TVANCPOS_HPOS_SHIFT);
                HWREG(baseAddr + C2TVANCSIZE) = (vsize << VPIF_C2TVANCSIZE_VSIZE_SHIFT) + (hsize << VPIF_C2TVANCSIZE_HSIZE_SHIFT);
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                HWREG(baseAddr + C2BHANCPOS) = (vpos << VPIF_C2BHANCPOS_VPOS_SHIFT) + (hpos << VPIF_C2BHANCPOS_HPOS_SHIFT);
                HWREG(baseAddr + C2BHANCSIZE) = (vsize << VPIF_C2BHANCSIZE_VSIZE_SHIFT) + (hsize << VPIF_C2BHANCSIZE_HSIZE_SHIFT);
            }
            if(hv==VPIF_VERTICAL)
            {
                HWREG(baseAddr + C2BVANCPOS) = (vpos << VPIF_C2BVANCPOS_VPOS_SHIFT) + (hpos << VPIF_C2BVANCPOS_HPOS_SHIFT);
                HWREG(baseAddr + C2BVANCSIZE) = (vsize << VPIF_C2BVANCSIZE_VSIZE_SHIFT) + (hsize << VPIF_C2BVANCSIZE_HSIZE_SHIFT);
            }
        }
    }
    if(channel==VPIF_CHANNEL_3)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                HWREG(baseAddr + C3THANCPOS) = (vpos << VPIF_C3THANCPOS_VPOS_SHIFT) + (hpos << VPIF_C3THANCPOS_HPOS_SHIFT);
                HWREG(baseAddr + C3THANCSIZE) = (vsize << VPIF_C3THANCSIZE_VSIZE_SHIFT) + (hsize << VPIF_C3THANCSIZE_HSIZE_SHIFT);
            }
            if(hv==VPIF_VERTICAL)
            {
                HWREG(baseAddr + C3TVANCPOS) = (vpos << VPIF_C3TVANCPOS_VPOS_SHIFT) + (hpos << VPIF_C3TVANCPOS_HPOS_SHIFT);
                HWREG(baseAddr + C3TVANCSIZE) = (vsize << VPIF_C3TVANCSIZE_VSIZE_SHIFT) + (hsize << VPIF_C3TVANCSIZE_HSIZE_SHIFT);
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(hv==VPIF_HORIZONTAL)
            {
                HWREG(baseAddr + C3BHANCPOS) = (vpos << VPIF_C3BHANCPOS_VPOS_SHIFT) + (hpos << VPIF_C3BHANCPOS_HPOS_SHIFT);
                HWREG(baseAddr + C3BHANCSIZE) = (vsize << VPIF_C3BHANCSIZE_VSIZE_SHIFT) + (hsize << VPIF_C3BHANCSIZE_HSIZE_SHIFT);
            }
            if(hv==VPIF_VERTICAL)
            {
                HWREG(baseAddr + C3BVANCPOS) = (vpos << VPIF_C3BVANCPOS_VPOS_SHIFT) + (hpos << VPIF_C3BVANCPOS_HPOS_SHIFT);
                HWREG(baseAddr + C3BVANCSIZE) = (vsize << VPIF_C3BVANCSIZE_VSIZE_SHIFT) + (hsize << VPIF_C3BVANCSIZE_HSIZE_SHIFT);
            }
        }
    }
}
/**
* \brief  This function configures the dimension of the video to be displayed
*          (both active portion and blanking portion). The application doesn't
*         need to specify dimension information, except when non-standard BT
*         video is to be displayed.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  mode       is the video standard (i.e., 480I or non-standard).
* \param  sdChannel  is the display channel (channel 3 or channel 2).
* \param  *buf         is the dimension information of the non-standard video to be displayed.
*                     It is of type vbufParam (structure).
*
* \return none.
**/
void VPIFDisplayModeConfig(unsigned int baseAddr, unsigned int mode, unsigned int sdChannel, VPIFVbufParam* buf)
{    /*
    typedef struct vbufParam
    {
    unsigned int sav2eav;
    unsigned int eav2sav;
    unsigned int vsize;
    unsigned int l1;
    unsigned int l3;
    unsigned int l5;
    unsigned int l7;
    unsigned int l9;
    unsigned int l11;
    } VPIFVbufParam;
    */
    if(mode==VPIF_480I)
    {
        if(sdChannel==VPIF_CHANNEL_2)
        {
            HWREG(baseAddr + C2HCFG) = (268 << VPIF_C2HCFG_EAV2SAV_SHIFT) | (1440 << VPIF_C2HCFG_SAV2EAV_SHIFT);
            HWREG(baseAddr + C2VCFG0) = (4 << VPIF_C2VCFG0_L1_SHIFT) | (20 << VPIF_C2VCFG0_L3_SHIFT);
            HWREG(baseAddr + C2VCFG1) = (264 << VPIF_C2VCFG1_L5_SHIFT) | (266 << VPIF_C2VCFG1_L7_SHIFT);
            HWREG(baseAddr + C2VCFG2) = (283 << VPIF_C2VCFG2_L9_SHIFT) | (1 << VPIF_C2VCFG2_L11_SHIFT);
            HWREG(baseAddr + C2VSIZE) = 525 << VPIF_C2VSIZE_VSIZE_SHIFT;
        }
        if(sdChannel==VPIF_CHANNEL_3)
        {
            HWREG(baseAddr + C3HCFG) = (268 << VPIF_C3HCFG_EAV2SAV_SHIFT) | (1440 << VPIF_C3HCFG_SAV2EAV_SHIFT);
            HWREG(baseAddr + C3VCFG0) = (4 << VPIF_C3VCFG0_L1_SHIFT) | (20 << VPIF_C3VCFG0_L3_SHIFT);
            HWREG(baseAddr + C3VCFG1) = (264 << VPIF_C3VCFG1_L5_SHIFT) | (266 << VPIF_C3VCFG1_L7_SHIFT);
            HWREG(baseAddr + C3VCFG2) = (283 << VPIF_C3VCFG2_L9_SHIFT) | (1 << VPIF_C3VCFG2_L11_SHIFT);
            HWREG(baseAddr + C3VSIZE) = 525 << VPIF_C3VSIZE_VSIZE_SHIFT;
        }
    }
    if(mode==VPIF_NONSTANDARD)
    {
        //TBD
        if(sdChannel==VPIF_CHANNEL_2);
        if(sdChannel==VPIF_CHANNEL_3);
    }
}
/**
* \brief  This function configures the frame buffer of the display video.
*         The buffer address (and offset when applicable) is passed to VPIF.

*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
* \param  field      is the field that the video is for (top/bottom).
* \param  yc         is the data component (luma/chroma).
* \param  addr         is the address of the frame buffer in the memory.
* \param  offset     is the line offset of the frame buffer in the memory.
*
* \return none.
**/
void VPIFDisplayFBConfig(unsigned int baseAddr, unsigned int channel, unsigned int field, unsigned yc, unsigned int addr, unsigned int offset)
{
    if(channel==VPIF_CHANNEL_2)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                HWREG(baseAddr + C2TLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                HWREG(baseAddr + C2TCHROMA) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                HWREG(baseAddr + C2BLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                HWREG(baseAddr + C2BCHROMA) = addr;
            }
        }
        HWREG(baseAddr + C2IMGOFFSET) = offset;
    }
    if(channel==VPIF_CHANNEL_3)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                HWREG(baseAddr + C3TLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                HWREG(baseAddr + C3TCHROMA) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                HWREG(baseAddr + C3BLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                HWREG(baseAddr + C3BCHROMA) = addr;
            }
        }
        HWREG(baseAddr + C3IMGOFFSET) = offset;
    }
}
/**
* \brief  This function exchanges the frame buffer of the displayed video.
*         A new buffer address (and offset when applicable) is passed to VPIF,
*         and the address of the previous buffer is read back.
*
* \param  baseAddr   is the Memory address of VPIF.
* \param  channel    is the display channel (channel 3 or channel 2).
* \param  field      is the field that the video is for (top/bottom).
* \param  yc         is the data component (luma/chroma).
* \param  addr         is the address of the frame buffer in the memory.
* \param  offset     is the line offset of the frame buffer in the memory.
*
* \return Previuos buffer address.
**/
unsigned int VPIFDisplayFBExchange(unsigned int baseAddr, unsigned int channel, unsigned int field, unsigned yc, unsigned int addr, unsigned int offset)
{
    unsigned int temp = 0;
    if(channel==VPIF_CHANNEL_2)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                temp = HWREG(baseAddr + C2TLUMA);
                HWREG(baseAddr + C2TLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                temp = HWREG(baseAddr + C2TCHROMA);
                HWREG(baseAddr + C2TCHROMA) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                temp = HWREG(baseAddr + C2BLUMA);
                HWREG(baseAddr + C2BLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                temp = HWREG(baseAddr + C2BCHROMA);
                HWREG(baseAddr + C2BCHROMA) = addr;
            }
        }
        HWREG(baseAddr + C2IMGOFFSET) = offset;
    }
    if(channel==VPIF_CHANNEL_3)
    {
        if(field==VPIF_TOP_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                temp = HWREG(baseAddr + C3TLUMA);
                HWREG(baseAddr + C3TLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                temp = HWREG(baseAddr + C3TCHROMA);
                HWREG(baseAddr + C3TCHROMA) = addr;
            }
        }
        if(field==VPIF_BOTTOM_FIELD)
        {
            if(yc==VPIF_LUMA)
            {
                temp = HWREG(baseAddr + C3BLUMA);
                HWREG(baseAddr + C3BLUMA) = addr;
            }
            if(yc==VPIF_CHROMA)
            {
                temp = HWREG(baseAddr + C3BCHROMA);
                HWREG(baseAddr + C3BCHROMA) = addr;
            }
        }
        HWREG(baseAddr + C3IMGOFFSET) = offset;
    }
    return temp;
}
