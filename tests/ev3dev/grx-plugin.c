// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

// GRX3 plugin to simulate LEGO MINDSTORMS EV3 screen for automated testing

#include <stdio.h>
#include <gmodule.h>
#include <grx-3.0.h>

#define EV3_SCREEN_WIDTH 178
#define EV3_SCREEN_HEIGHT 128

// Private symbols from GRX library
extern  struct _GR_driverInfo  _GrDriverInfo;
extern GrxVideoMode *_gr_select_mode(GrxVideoDriver *drv,int w,int h,int bpp,
                                     int txt,unsigned int *ep);


// Empty device manager implementation

#define TEST_TYPE_DEVICE_MANAGER test_device_manager_get_type()
G_DECLARE_FINAL_TYPE(TestDeviceManager, test_device_manager, TEST, DEVICE_MANAGER, GrxDeviceManager)

struct _TestDeviceManager
{
    GrxDeviceManager parent_instance;
};

G_DEFINE_TYPE(TestDeviceManager, test_device_manager, GRX_TYPE_DEVICE_MANAGER)

static void test_device_manager_class_init(TestDeviceManagerClass *klass)
{
}

static void test_device_manager_init(TestDeviceManager *self)
{
}


// GRX3 plugin implementation

static guint8 framebuffer[EV3_SCREEN_WIDTH * EV3_SCREEN_HEIGHT * 4];

static gboolean mem_setmode(GrxVideoMode *mp, int noclear)
{
     return TRUE;
}

static GrxVideoModeExt ext = {
    .mode   = GRX_FRAME_MODE_LFB_32BPP_LOW,
    .frame  = framebuffer,
    .cprec  = { 8, 8, 8 },
    .cpos   = { 0, 8, 16 },
    .flags  = GRX_VIDEO_MODE_FLAG_MEMORY,
    .setup  = mem_setmode,
};

static GrxVideoMode modes[] = {
    {
        .present        = TRUE,
        .bpp            = 32,
        .width          = EV3_SCREEN_WIDTH,
        .height         = EV3_SCREEN_HEIGHT,
        .line_offset    = EV3_SCREEN_WIDTH * 4,
        .extended_info  = &ext,
    },
};

static gboolean init(const char *options)
{
    TestDeviceManager *device_manager;

    device_manager = g_object_new(TEST_TYPE_DEVICE_MANAGER, NULL);
    _GrDriverInfo.device_manager = GRX_DEVICE_MANAGER(device_manager);

    return TRUE;
}

static void reset(void)
{
}

G_MODULE_EXPORT GrxVideoDriver grx_test_video_driver = {
    .name        = "test",
    .modes       = modes,
    .n_modes     = G_N_ELEMENTS(modes),
    .init        = init,
    .reset       = reset,
    .select_mode = _gr_select_mode,
};
