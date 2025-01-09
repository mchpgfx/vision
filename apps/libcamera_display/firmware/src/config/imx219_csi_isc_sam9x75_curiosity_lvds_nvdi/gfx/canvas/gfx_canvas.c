// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2020 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

#include <stdlib.h>

#include "gfx/driver/gfx_driver.h"
#include "gfx/canvas/gfx_canvas_api.h"
#include "gfx/canvas/gfx_canvas.h"

#include "definitions.h"


/*******************************************************************************
  GFX Canvas

  Company:
    Microchip Technology Inc.

  File Name:
    gfx_canvas.c

  Summary:
    GFX Canvas Virtual Display Component private header file

  Description:
    The GFX Canvas provides a virtual display driver interface.

  Remarks:
    None
*******************************************************************************/

typedef enum
{
    GFXC_INIT,
    GFXC_RUNNING,
} GFXC_STATE;

//LE Driver interface function prototypes
static void GFXC_Update(void);
static gfxResult GFXC_BlitBuffer(int32_t x, int32_t y, gfxPixelBuffer* buf);
static gfxDriverIOCTLResponse GFX_CANVAS_IOCTL(gfxDriverIOCTLRequest request, void* arg);

GFXC_CANVAS canvas[CONFIG_NUM_CANVAS_OBJ];
unsigned int numLayers = CONFIG_NUM_LAYERS;

static unsigned int activeCanvasID = 0;
static unsigned int baseCanvasID = 0;
static GFXC_STATE gfxcState;


static unsigned int displayWidth = 0;
static unsigned int displayHeight = 0;


const gfxGraphicsProcessor * gfxcGPU = NULL;

const gfxDisplayDriver * gfxDispCtrlr = &xlcdcDisplayDriver;

const gfxDisplayDriver gfxDriverInterface =
{
    .update = GFXC_Update,
    .blitBuffer = GFXC_BlitBuffer,
    .ioctl = GFX_CANVAS_IOCTL
};


static void gfxcObjectsInitialize(void)
{
    unsigned int id;

    id = gfxcCreate();
    gfxcSetPixelBuffer(id, 1280, 720, GFX_COLOR_MODE_RGB_565,
                       NULL);
}



void GFX_CANVAS_Initialize(void)
{
    unsigned int i;
    gfxcState = GFXC_INIT;
    baseCanvasID = 0;

    //Initialize canvas objects
    for (i = 0; i < CONFIG_NUM_CANVAS_OBJ; i++)
    {
        canvas[i].pixelBuffer.mode = CANVAS_DEFAULT_FORMAT;
        canvas[i].id = CANVAS_ID_INVALID;
        canvas[i].layer.id = LAYER_ID_INVALID;
        canvas[i].effects.cb = NULL;
        canvas[i].effects.fade.status = GFXC_FX_IDLE;
        canvas[i].effects.move.status = GFXC_FX_IDLE;
    }

    if (gfxDispCtrlr != NULL)
    {
        gfxIOCTLArg_DisplaySize disp;

        gfxDispCtrlr->ioctl(GFX_IOCTL_GET_DISPLAY_SIZE, (gfxIOCTLArg_DisplaySize *) &disp);
        displayWidth = disp.width;
        displayHeight = disp.height;
    }

    gfxcObjectsInitialize();
}

gfxResult GFXC_FrameStart(uint32_t reserved)
{
    return GFX_SUCCESS;
}

gfxResult GFXC_FrameEnd(void)
{
    return GFX_SUCCESS;
}

uint32_t GFXC_GetVSYNCCount(void)
{
    return 0;
}

void GFXC_Update()
{
}

gfxPixelBuffer * GFXC_GetPixelBuffer(int32_t idx)
{
    return &canvas[baseCanvasID + activeCanvasID].pixelBuffer;
}

static gfxResult GFXC_BufferBlit(const gfxPixelBuffer* source,
                            const gfxRect* rectSrc,
                            const gfxPixelBuffer* dest,
                            const gfxRect* rectDest)
{
    void* srcPtr;
    void* destPtr;
    uint32_t row, rowSize;
    unsigned int width, height;
    width = (rectSrc->width < rectDest->width) ?
                 rectSrc->width : rectDest->width;
    height = (rectSrc->height < rectDest->height) ?
                 rectSrc->height : rectDest->height;
    rowSize = width * gfxColorInfoTable[dest->mode].size;

    for(row = 0; row < height; row++)
    {
        srcPtr = gfxPixelBufferOffsetGet(source, rectSrc->x, rectSrc->y + row);
        destPtr = gfxPixelBufferOffsetGet(dest, rectDest->x, rectDest->y + row);

        memcpy(destPtr, srcPtr, rowSize);
    }

    return GFX_SUCCESS;
}

gfxResult GFXC_BlitBuffer(int32_t x,
                          int32_t y,
                          gfxPixelBuffer* buf)
{
    gfxRect srcRect, destRect;

	if (gfxcState != GFXC_RUNNING)
	{
		return GFX_FAILURE;
	}

	srcRect.x = 0;
    srcRect.y = 0;
    srcRect.height = buf->size.height;
    srcRect.width = buf->size.width;

    destRect.x = x;
    destRect.y = y;
    destRect.height = buf->size.height;
    destRect.width = buf->size.width;

	GFXC_BufferBlit(buf,
					&srcRect,
					&canvas[baseCanvasID + activeCanvasID].pixelBuffer,
					&destRect);


    return GFX_SUCCESS;
}

GFXC_RESULT _gfxcCopyBuffer(unsigned int srcID,
                         unsigned int destID,
                         const gfxRect * srcRect,
                         const gfxRect * destRect)
{
    void* srcPtr;
    void* destPtr;
    uint32_t row, rowSize;

    if (canvas[srcID].pixelBuffer.mode != canvas[destID].pixelBuffer.mode ||
            srcRect->height != destRect->height ||
            srcRect->width != destRect->width)
        return GFX_FAILURE;

    rowSize = canvas[srcID].pixelBuffer.size.width *
              gfxColorInfoTable[canvas[srcID].pixelBuffer.mode].size;

    for(row = 0; row < canvas[srcID].pixelBuffer.size.height; row++)
    {
        srcPtr = gfxPixelBufferOffsetGet(&canvas[srcID].pixelBuffer,
                                          srcRect->x, srcRect->y + row);
        destPtr = gfxPixelBufferOffsetGet(&canvas[destID].pixelBuffer,
                                          destRect->x, destRect->y + row);

        memcpy(destPtr, srcPtr, rowSize);
    }

    return GFX_SUCCESS;

}

GFXC_RESULT _gfxcSetBaseCanvasID(uint32_t base)
{
    if (baseCanvasID >= CONFIG_NUM_CANVAS_OBJ)
        return GFX_FAILURE;

    baseCanvasID = base;

    return GFX_SUCCESS;
}

gfxResult GFXC_GetLayerState(uint32_t idx, gfxLayerState* st)
{
    st->rect.x = canvas[baseCanvasID + idx].layer.pos.xpos;
    st->rect.y = canvas[baseCanvasID + idx].layer.pos.ypos;
    st->rect.width = canvas[baseCanvasID + idx].layer.size.width;
    st->rect.height = canvas[baseCanvasID + idx].layer.size.height;
    st->enabled = canvas[baseCanvasID + idx].active;

    return GFX_SUCCESS;
}

GFXC_RESULT _gfxcCanvasUpdate(unsigned int canvasID)
{
    gfxIOCTLArg_Value argValue;
    gfxIOCTLArg_LayerValue setBaseAddressParm;
    gfxIOCTLArg_LayerValue setLayerEnabledParm;
    gfxIOCTLArg_LayerValue setColorModeParm;
    gfxIOCTLArg_LayerSize setResParm;
    gfxIOCTLArg_LayerPosition setPositionParm;
    gfxIOCTLArg_LayerSize setSizeParm;
    gfxIOCTLArg_LayerValue setAlphaParm;

    if (canvasID < CONFIG_NUM_CANVAS_OBJ &&
        canvas[canvasID].layer.id != LAYER_ID_INVALID &&
        gfxDispCtrlr != NULL &&
        gfxDispCtrlr->ioctl != NULL)
    {
        setBaseAddressParm.layer.id = canvas[canvasID].layer.id;
        if (canvas[canvasID].pixelBuffer.pixels != NULL)
            setBaseAddressParm.value.v_pointer = canvas[canvasID].pixelBuffer.pixels;
        else
        {
            argValue.value.v_int = canvas[canvasID].layer.id;
            gfxDispCtrlr->ioctl(GFX_IOCTL_SET_ACTIVE_LAYER, (gfxIOCTLArg_Value*) &argValue);

            gfxDispCtrlr->ioctl(GFX_IOCTL_GET_FRAMEBUFFER, (gfxIOCTLArg_Value*) &argValue);
            setBaseAddressParm.value.v_pointer = argValue.value.v_pbuffer->pixels;
        }

        setColorModeParm.layer.id = canvas[canvasID].layer.id;
        setColorModeParm.value.v_uint = (unsigned int) canvas[canvasID].pixelBuffer.mode;

		//This is the physical resolution of the layer pixel buffer
        setResParm.layer.id = canvas[canvasID].layer.id;
        setResParm.width = canvas[canvasID].pixelBuffer.size.width;
        setResParm.height = canvas[canvasID].pixelBuffer.size.height;

        setSizeParm.layer.id = canvas[canvasID].layer.id;
        setSizeParm.width = canvas[canvasID].layer.size.width;
        setSizeParm.height = canvas[canvasID].layer.size.height;

        setPositionParm.layer.id = canvas[canvasID].layer.id;
        setPositionParm.x = canvas[canvasID].layer.pos.xpos;
        setPositionParm.y = canvas[canvasID].layer.pos.ypos;

        setAlphaParm.layer.id = canvas[canvasID].layer.id;
        setAlphaParm.value.v_uint = canvas[canvasID].layer.alpha;

		//align offsets for non-32bpp frames
        if (gfxColorInfoTable[canvas[canvasID].pixelBuffer.mode].size != 4)
            setPositionParm.x &= ~0x3;

        if (setPositionParm.x < 0)
        {
            setBaseAddressParm.value.v_pointer += abs(setPositionParm.x) *
                    gfxColorInfoTable[canvas[canvasID].pixelBuffer.mode].size;

            setSizeParm.width += setPositionParm.x;
            setPositionParm.x = 0;
        }

        if (setPositionParm.y < 0)
        {
            setBaseAddressParm.value.v_pointer += abs(setPositionParm.y) *
                    setResParm.width *
                    gfxColorInfoTable[canvas[canvasID].pixelBuffer.mode].size;

            setSizeParm.height += setPositionParm.y;
            setPositionParm.y = 0;
        }

        if (setPositionParm.x + setSizeParm.width > displayWidth)
            setSizeParm.width = displayWidth - setPositionParm.x;

        if (setPositionParm.y + setSizeParm.height > displayHeight)
            setSizeParm.height = displayHeight - setPositionParm.y;

        //Lock layer and apply layer properties
        gfxDispCtrlr->ioctl(GFX_IOCTL_SET_LAYER_LOCK, (gfxIOCTLArg_LayerValue *) &setBaseAddressParm);

        if (setBaseAddressParm.value.v_pointer != NULL)
            gfxDispCtrlr->ioctl(GFX_IOCTL_SET_LAYER_BASE_ADDRESS, (gfxIOCTLArg_LayerValue *) &setBaseAddressParm);

        gfxDispCtrlr->ioctl(GFX_IOCTL_SET_LAYER_COLOR_MODE, (gfxIOCTLArg_LayerValue *) &setColorModeParm);
        gfxDispCtrlr->ioctl(GFX_IOCTL_SET_LAYER_SIZE, (gfxIOCTLArg_LayerSize *) &setResParm);
        gfxDispCtrlr->ioctl(GFX_IOCTL_SET_LAYER_POSITION, (void *) &setPositionParm);
        gfxDispCtrlr->ioctl(GFX_IOCTL_SET_LAYER_WINDOW_SIZE, (void *) &setSizeParm);

        gfxDispCtrlr->ioctl(GFX_IOCTL_SET_LAYER_ALPHA, (void *) &setAlphaParm);

        setLayerEnabledParm.layer.id = canvas[canvasID].layer.id;
        setLayerEnabledParm.value.v_bool = canvas[canvasID].active;
        gfxDispCtrlr->ioctl(GFX_IOCTL_SET_LAYER_ENABLED, (gfxIOCTLArg_LayerValue *) &setLayerEnabledParm);


        //Unlock layer to affect changes
        gfxDispCtrlr->ioctl(GFX_IOCTL_SET_LAYER_UNLOCK, (gfxIOCTLArg_LayerValue *) &setBaseAddressParm);

        return GFX_SUCCESS;
    }

    return GFX_FAILURE;
}



GFXC_STATUS _gfxcGetStatus(void)
{
    gfxIOCTLArg_Value val;

    GFX_CANVAS_IOCTL(GFX_IOCTL_GET_STATUS, &val);

    return (val.value.v_uint == 0) ? GFXC_STAT_IDLE : GFXC_STAT_BUSY;
}

void GFX_CANVAS_Task(void)
{
    switch(gfxcState)
    {
        case GFXC_INIT:
        {
            gfxcState = GFXC_RUNNING;
            break;
        }
        case GFXC_RUNNING:
        {
            break;
        }
        default:
            break;
    }
}

gfxDriverIOCTLResponse GFX_CANVAS_IOCTL(gfxDriverIOCTLRequest request,
                                      void* arg)
{
    gfxIOCTLArg_Value* val;
    gfxIOCTLArg_DisplaySize* disp;
    gfxIOCTLArg_LayerRect* rect;

    switch(request)
    {
        case GFX_IOCTL_GET_BUFFER_COUNT:
        {
            val = (gfxIOCTLArg_Value*)arg;

            val->value.v_uint = CONFIG_BUFFER_PER_LAYER;

            return GFX_IOCTL_OK;
        }
        case GFX_IOCTL_GET_DISPLAY_SIZE:
        {
            disp = (gfxIOCTLArg_DisplaySize*)arg;

            disp->width = canvas[baseCanvasID + activeCanvasID].pixelBuffer.size.width;
            disp->height = canvas[baseCanvasID + activeCanvasID].pixelBuffer.size.height;

            return GFX_IOCTL_OK;
        }
        case GFX_IOCTL_GET_LAYER_COUNT:
        {
            val = (gfxIOCTLArg_Value*)arg;

            val->value.v_uint = CONFIG_NUM_CANVAS_OBJ - baseCanvasID;

            return GFX_IOCTL_OK;
        }
        case GFX_IOCTL_GET_ACTIVE_LAYER:
        {
            val = (gfxIOCTLArg_Value*)arg;

            val->value.v_uint = activeCanvasID;

            return GFX_IOCTL_OK;
        }
        case GFX_IOCTL_SET_ACTIVE_LAYER:
        {
            val = (gfxIOCTLArg_Value*)arg;

            if(val->value.v_uint >= baseCanvasID + CONFIG_NUM_CANVAS_OBJ)
            {
                return GFX_IOCTL_ERROR_UNKNOWN;
            }
            else
            {
                activeCanvasID = val->value.v_uint;

                return GFX_IOCTL_OK;
            }
        }
        case GFX_IOCTL_GET_LAYER_RECT:
        {
            rect = (gfxIOCTLArg_LayerRect*)arg;

            if(rect->layer.id >= baseCanvasID + CONFIG_NUM_CANVAS_OBJ)
                return GFX_IOCTL_ERROR_UNKNOWN;

            rect->x = canvas[baseCanvasID + rect->layer.id].layer.pos.xpos;
            rect->y = canvas[baseCanvasID + rect->layer.id].layer.pos.ypos;
            rect->width = canvas[baseCanvasID + rect->layer.id].layer.size.width;
            rect->height = canvas[baseCanvasID + rect->layer.id].layer.size.height;

            return GFX_IOCTL_OK;
        }
        case GFX_IOCTL_GET_VSYNC_COUNT:
        {
            val = (gfxIOCTLArg_Value*)arg;

            val->value.v_uint = 0;

            return GFX_IOCTL_OK;
        }
        case GFX_IOCTL_GET_FRAMEBUFFER:
        {
            val = (gfxIOCTLArg_Value*)arg;

            val->value.v_pbuffer = &canvas[baseCanvasID + activeCanvasID].pixelBuffer;

            return GFX_IOCTL_OK;
        }
        case GFX_IOCTL_SET_PALETTE:
        {
            //Global Palette is a controller function, pass it down to the controller
            if (gfxDispCtrlr != NULL)
                return gfxDispCtrlr->ioctl(GFX_IOCTL_SET_PALETTE, (gfxIOCTLArg_Palette*) arg);
            else
                return GFX_IOCTL_UNSUPPORTED;
        }
        case GFX_IOCTL_GET_STATUS:
        {
            unsigned int i = CANVAS_ID_INVALID;
            val = (gfxIOCTLArg_Value*)arg;

            val->value.v_uint = 0;

            //Try to allocate from static canvas objects
            for (i = 0; i < CONFIG_NUM_CANVAS_OBJ; i++)
            {
                if (canvas[i].effects.fade.status != GFXC_FX_IDLE ||
                    canvas[i].effects.move.status != GFXC_FX_IDLE)
                {
                    val->value.v_uint = 1;
                    break;
                }

                if (canvas[i].active == GFX_TRUE)
                {
                    if (gfxDispCtrlr != NULL)
                        return gfxDispCtrlr->ioctl(GFX_IOCTL_GET_STATUS,
                                                   (gfxIOCTLArg_Value*) arg);
                }
            }

            return GFX_IOCTL_OK;
        }
        default:
        {
            if (request >= GFX_IOCTL_LAYER_REQ_START &&
                request < GFX_IOCTL_LAYER_REQ_END)
            {
                return gfxDispCtrlr->ioctl(request, (gfxIOCTLArg_LayerArg*)arg);
            }
        }
    }

    return GFX_IOCTL_UNSUPPORTED;
}

/* *****************************************************************************
 End of File
 */
