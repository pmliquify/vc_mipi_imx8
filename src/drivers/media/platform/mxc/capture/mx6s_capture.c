/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2014-2016 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2019 NXP
 */

/*!
 * @file mx6s_csi.c
 *
 * @brief mx6sx CMOS Sensor interface functions
 *
 * @ingroup CSI
 */

#define VC_MIPI     1   /* CCC - enables VC MIPI driver code */

#include <asm/dma.h>
#include <linux/busfreq-imx.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gcd.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/mfd/syscon.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/media-bus-format.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>

#if VC_MIPI
  #include "vc_mipi.h"
  #include "vc_mipi_user_ctrl.h"
#endif

#define MX6S_CAM_DRV_NAME "mx6s-csi"
#define MX6S_CAM_VERSION "0.0.1"
#define MX6S_CAM_DRIVER_DESCRIPTION "i.MX6S_CSI"

#define MAX_VIDEO_MEM 64

#if VC_MIPI
/*
 * Increase the video memory size to 160 so that we
 * get upto 6 buffers for 13MP.
 */
  #undef  MAX_VIDEO_MEM
//  #define MAX_VIDEO_MEM 160
  #define MAX_VIDEO_MEM 256
//  #define MAX_VIDEO_MEM 320
#endif

/* reset values */
#define CSICR1_RESET_VAL    0x40000800
#define CSICR2_RESET_VAL    0x0
#define CSICR3_RESET_VAL    0x0

/* csi control reg 1 */
#define BIT_SWAP16_EN       (0x1 << 31)
#define BIT_EXT_VSYNC       (0x1 << 30)
#define BIT_EOF_INT_EN      (0x1 << 29)
#define BIT_PRP_IF_EN       (0x1 << 28)
#define BIT_CCIR_MODE       (0x1 << 27)
#define BIT_COF_INT_EN      (0x1 << 26)
#define BIT_SF_OR_INTEN     (0x1 << 25)
#define BIT_RF_OR_INTEN     (0x1 << 24)
#define BIT_SFF_DMA_DONE_INTEN  (0x1 << 22)
#define BIT_STATFF_INTEN    (0x1 << 21)
#define BIT_FB2_DMA_DONE_INTEN  (0x1 << 20)
#define BIT_FB1_DMA_DONE_INTEN  (0x1 << 19)
#define BIT_RXFF_INTEN      (0x1 << 18)
#define BIT_SOF_POL     (0x1 << 17)
#define BIT_SOF_INTEN       (0x1 << 16)
#define BIT_MCLKDIV     (0xF << 12)
#define BIT_HSYNC_POL       (0x1 << 11)
#define BIT_CCIR_EN     (0x1 << 10)
#define BIT_MCLKEN      (0x1 << 9)
#define BIT_FCC         (0x1 << 8)
#define BIT_PACK_DIR        (0x1 << 7)
#define BIT_CLR_STATFIFO    (0x1 << 6)
#define BIT_CLR_RXFIFO      (0x1 << 5)
#define BIT_GCLK_MODE       (0x1 << 4)
#define BIT_INV_DATA        (0x1 << 3)
#define BIT_INV_PCLK        (0x1 << 2)
#define BIT_REDGE       (0x1 << 1)
#define BIT_PIXEL_BIT       (0x1 << 0)

#define SHIFT_MCLKDIV       12

/* control reg 3 */
#define BIT_FRMCNT      (0xFFFF << 16)
#define BIT_FRMCNT_RST      (0x1 << 15)
#define BIT_DMA_REFLASH_RFF (0x1 << 14)
#define BIT_DMA_REFLASH_SFF (0x1 << 13)
#define BIT_DMA_REQ_EN_RFF  (0x1 << 12)
#define BIT_DMA_REQ_EN_SFF  (0x1 << 11)
#define BIT_STATFF_LEVEL    (0x7 << 8)
#define BIT_HRESP_ERR_EN    (0x1 << 7)
#define BIT_RXFF_LEVEL      (0x7 << 4)
#define BIT_TWO_8BIT_SENSOR (0x1 << 3)
#define BIT_ZERO_PACK_EN    (0x1 << 2)
#define BIT_ECC_INT_EN      (0x1 << 1)
#define BIT_ECC_AUTO_EN     (0x1 << 0)

#define SHIFT_FRMCNT        16
#define SHIFT_RXFIFO_LEVEL  4

/* csi status reg */
#define BIT_ADDR_CH_ERR_INT (0x1 << 28)
#define BIT_FIELD0_INT      (0x1 << 27)
#define BIT_FIELD1_INT      (0x1 << 26)
#define BIT_SFF_OR_INT      (0x1 << 25)
#define BIT_RFF_OR_INT      (0x1 << 24)
#define BIT_DMA_TSF_DONE_SFF    (0x1 << 22)
#define BIT_STATFF_INT      (0x1 << 21)
#define BIT_DMA_TSF_DONE_FB2    (0x1 << 20)
#define BIT_DMA_TSF_DONE_FB1    (0x1 << 19)
#define BIT_RXFF_INT        (0x1 << 18)
#define BIT_EOF_INT     (0x1 << 17)
#define BIT_SOF_INT     (0x1 << 16)
#define BIT_F2_INT      (0x1 << 15)
#define BIT_F1_INT      (0x1 << 14)
#define BIT_COF_INT     (0x1 << 13)
#define BIT_HRESP_ERR_INT   (0x1 << 7)
#define BIT_ECC_INT     (0x1 << 1)
#define BIT_DRDY        (0x1 << 0)

/* csi control reg 18 */
#define BIT_CSI_ENABLE          (0x1 << 31)
#define BIT_MIPI_DATA_FORMAT_RAW8       (0x2a << 25)
#define BIT_MIPI_DATA_FORMAT_RAW10      (0x2b << 25)
#if VC_MIPI
#define BIT_MIPI_DATA_FORMAT_RAW12      (0x2c << 25)
#endif
#define BIT_MIPI_DATA_FORMAT_YUV422_8B  (0x1e << 25)
#define BIT_MIPI_DATA_FORMAT_MASK   (0x3F << 25)
#define BIT_MIPI_DATA_FORMAT_OFFSET 25
#define BIT_DATA_FROM_MIPI      (0x1 << 22)
#define BIT_MIPI_YU_SWAP        (0x1 << 21)
#define BIT_MIPI_DOUBLE_CMPNT   (0x1 << 20)
#define BIT_BASEADDR_CHG_ERR_EN (0x1 << 9)
#define BIT_BASEADDR_SWITCH_SEL (0x1 << 5)
#define BIT_BASEADDR_SWITCH_EN  (0x1 << 4)
#define BIT_PARALLEL24_EN       (0x1 << 3)
#define BIT_DEINTERLACE_EN      (0x1 << 2)
#define BIT_TVDECODER_IN_EN     (0x1 << 1)
#define BIT_NTSC_EN             (0x1 << 0)

#define CSI_MCLK_VF     1
#define CSI_MCLK_ENC        2
#define CSI_MCLK_RAW        4
#define CSI_MCLK_I2C        8

#define CSI_CSICR1      0x0
#define CSI_CSICR2      0x4
#define CSI_CSICR3      0x8
#define CSI_STATFIFO        0xC
#define CSI_CSIRXFIFO       0x10
#define CSI_CSIRXCNT        0x14
#define CSI_CSISR       0x18

#define CSI_CSIDBG      0x1C
#define CSI_CSIDMASA_STATFIFO   0x20
#define CSI_CSIDMATS_STATFIFO   0x24
#define CSI_CSIDMASA_FB1    0x28
#define CSI_CSIDMASA_FB2    0x2C
#define CSI_CSIFBUF_PARA    0x30
#define CSI_CSIIMAG_PARA    0x34


#define CSI_CSICR18     0x48
#define CSI_CSICR19     0x4c

#define NUM_FORMATS ARRAY_SIZE(formats)
#define MX6SX_MAX_SENSORS    1

struct csi_signal_cfg_t {
    unsigned data_width:3;
    unsigned clk_mode:2;
    unsigned ext_vsync:1;
    unsigned Vsync_pol:1;
    unsigned Hsync_pol:1;
    unsigned pixclk_pol:1;
    unsigned data_pol:1;
    unsigned sens_clksrc:1;
};

struct csi_config_t {
    /* control reg 1 */
    unsigned int swap16_en:1;
    unsigned int ext_vsync:1;
    unsigned int eof_int_en:1;
    unsigned int prp_if_en:1;
    unsigned int ccir_mode:1;
    unsigned int cof_int_en:1;
    unsigned int sf_or_inten:1;
    unsigned int rf_or_inten:1;
    unsigned int sff_dma_done_inten:1;
    unsigned int statff_inten:1;
    unsigned int fb2_dma_done_inten:1;
    unsigned int fb1_dma_done_inten:1;
    unsigned int rxff_inten:1;
    unsigned int sof_pol:1;
    unsigned int sof_inten:1;
    unsigned int mclkdiv:4;
    unsigned int hsync_pol:1;
    unsigned int ccir_en:1;
    unsigned int mclken:1;
    unsigned int fcc:1;
    unsigned int pack_dir:1;
    unsigned int gclk_mode:1;
    unsigned int inv_data:1;
    unsigned int inv_pclk:1;
    unsigned int redge:1;
    unsigned int pixel_bit:1;

    /* control reg 3 */
    unsigned int frmcnt:16;
    unsigned int frame_reset:1;
    unsigned int dma_reflash_rff:1;
    unsigned int dma_reflash_sff:1;
    unsigned int dma_req_en_rff:1;
    unsigned int dma_req_en_sff:1;
    unsigned int statff_level:3;
    unsigned int hresp_err_en:1;
    unsigned int rxff_level:3;
    unsigned int two_8bit_sensor:1;
    unsigned int zero_pack_en:1;
    unsigned int ecc_int_en:1;
    unsigned int ecc_auto_en:1;
    /* fifo counter */
    unsigned int rxcnt;
};

/*
 * Basic structures
 */
struct mx6s_fmt {
    char  name[32];
    u32   fourcc;       /* v4l2 format id */
    u32   pixelformat;
    u32   mbus_code;
    int   bpp;
};

static struct mx6s_fmt formats[] = {
    {
        .name       = "UYVY-16",
        .fourcc     = V4L2_PIX_FMT_UYVY,
        .pixelformat= V4L2_PIX_FMT_UYVY,
        .mbus_code  = MEDIA_BUS_FMT_UYVY8_2X8,
        .bpp        = 2,
    }, {
        .name       = "YUYV-16",
        .fourcc     = V4L2_PIX_FMT_YUYV,
        .pixelformat= V4L2_PIX_FMT_YUYV,
        .mbus_code  = MEDIA_BUS_FMT_YUYV8_2X8,
        .bpp        = 2,
    }, {
        .name       = "YUV32 (X-Y-U-V)",
        .fourcc     = V4L2_PIX_FMT_YUV32,
        .pixelformat= V4L2_PIX_FMT_YUV32,
        .mbus_code  = MEDIA_BUS_FMT_AYUV8_1X32,
        .bpp        = 4,
    }, {
        .name       = "RAWRGB8 (SBGGR8)",
        .fourcc     = V4L2_PIX_FMT_SBGGR8,
        .pixelformat= V4L2_PIX_FMT_SBGGR8,
        .mbus_code  = MEDIA_BUS_FMT_SBGGR8_1X8,
        .bpp        = 1,
    }
    ,{
        .name       = "RAWRGB8 (SGRBG8)",
        .fourcc     = V4L2_PIX_FMT_SGRBG8,
        .pixelformat= V4L2_PIX_FMT_SGRBG8,
        .mbus_code  = MEDIA_BUS_FMT_SGRBG8_1X8,
        .bpp        = 1,
    }
    ,{
        .name       = "RAWRGB10 (SGRBG10)",
        .fourcc     = V4L2_PIX_FMT_SGRBG10,
        .pixelformat= V4L2_PIX_FMT_SGRBG10,
        .mbus_code  = MEDIA_BUS_FMT_SGRBG10_1X10,
        .bpp        = RAW10_BPP,  // 1, 2
    }
#if VC_MIPI
    ,{
        .name       = "Gray8 (GREY)",
        .fourcc     = V4L2_PIX_FMT_GREY,
        .pixelformat= V4L2_PIX_FMT_GREY,
        .mbus_code  = MEDIA_BUS_FMT_Y8_1X8, // MEDIA_BUS_FMT_SBGGR8_1X8,
//        .mbus_code  = MEDIA_BUS_FMT_SBGGR8_1X8,
        .bpp        = 1,
    }
    ,{
        .name       = "Gray10 (Y10)",
        .fourcc     = V4L2_PIX_FMT_Y10,
        .pixelformat= V4L2_PIX_FMT_Y10,
        .mbus_code  = MEDIA_BUS_FMT_Y10_1X10,
        .bpp        = RAW10_BPP,  // 1, 2
    }
    ,{
        .name       = "Gray12 (Y12)",
        .fourcc     = V4L2_PIX_FMT_Y12,
        .pixelformat= V4L2_PIX_FMT_Y12,
        .mbus_code  = MEDIA_BUS_FMT_Y12_1X12,
        .bpp        = RAW12_BPP,  // 1, 2,
    }
    ,{
        .name       = "RAWRGB8 (SRGGB8)",
        .fourcc     = V4L2_PIX_FMT_SRGGB8,
        .pixelformat= V4L2_PIX_FMT_SRGGB8,
        .mbus_code  = MEDIA_BUS_FMT_SRGGB8_1X8,
        .bpp        = 1,
    }
    ,{
        .name       = "RAWRGB10 (SRGGB10)",
        .fourcc     = V4L2_PIX_FMT_SRGGB10,
        .pixelformat= V4L2_PIX_FMT_SRGGB10,
        .mbus_code  = MEDIA_BUS_FMT_SRGGB10_1X10,
        .bpp        = RAW10_BPP,  // 1, 2
    }
    ,{
        .name       = "RAWRGB12 (SRGGB12)",
        .fourcc     = V4L2_PIX_FMT_SRGGB12,
        .pixelformat= V4L2_PIX_FMT_SRGGB12,
        .mbus_code  = MEDIA_BUS_FMT_SRGGB12_1X12,
        .bpp        = RAW12_BPP,  // 1, 2,
    }
    ,{
        .name       = "RAWRGB10 (SGBRG10)",
        .fourcc     = V4L2_PIX_FMT_SGBRG10,
        .pixelformat= V4L2_PIX_FMT_SGBRG10,
        .mbus_code  = MEDIA_BUS_FMT_SGBRG10_1X10,
        .bpp        = RAW10_BPP,  // 1, 2
    }

#endif
};

struct mx6s_buf_internal {
    struct list_head    queue;
    int                 bufnum;
    bool                discard;
};

/* buffer for one video frame */
struct mx6s_buffer {
    /* common v4l buffer stuff -- must be first */
    struct vb2_v4l2_buffer          vb;
    struct mx6s_buf_internal    internal;
};

struct mx6s_csi_mux {
    struct regmap *gpr;
    u8 req_gpr;
    u8 req_bit;
};

struct mx6s_csi_soc {
    bool rx_fifo_rst;
    int baseaddr_switch;
};

struct mx6s_csi_dev {
    struct device       *dev;
    struct video_device *vdev;
    struct v4l2_subdev  *sd;
    struct v4l2_device  v4l2_dev;

    struct vb2_queue            vb2_vidq;
    struct v4l2_ctrl_handler    ctrl_handler;

    struct mutex        lock;
    spinlock_t          slock;

    int open_count;

    /* clock */
    struct clk  *clk_disp_axi;
    struct clk  *clk_disp_dcic;
    struct clk  *clk_csi_mclk;

    void __iomem *regbase;
    int irq;

    u32      nextfb;
    u32      skipframe;
    u32  type;
    u32 bytesperline;
    v4l2_std_id std;
    struct mx6s_fmt     *fmt;
    struct v4l2_pix_format pix;
    u32 mbus_code;

    unsigned int frame_count;

    struct list_head    capture;
    struct list_head    active_bufs;
    struct list_head    discard;

    void                        *discard_buffer;
    dma_addr_t                  discard_buffer_dma;
    size_t                      discard_size;
    struct mx6s_buf_internal    buf_discard[2];

    struct v4l2_async_subdev    asd;
    struct v4l2_async_notifier  subdev_notifier;

    bool csi_mipi_mode;
    bool csi_two_8bit_sensor_mode;
    const struct mx6s_csi_soc *soc;
    struct mx6s_csi_mux csi_mux;

#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD2
    struct v4l2_ctrl        *ctrls[MAX_NUM_CTRLS];
#endif
};

/*----------------------------------------------------------------------*/
#if VC_MIPI     /* [[[ - VC MIPI code */
/*----------------------------------------------------------------------*/
// static struct v4l2_pix_format mx6s_pix_fmt = { 0 };
static struct mx6s_csi_dev *vc_mipi_csi_dev = NULL;

/****** mx6s_get_pix_fmt = Get pixel format = 12.2020 *******************/
void mx6s_get_pix_fmt(struct v4l2_pix_format *pix)
{

//    if(pix)
//      memcpy(pix, &mx6s_pix_fmt, sizeof(struct v4l2_pix_format));

    if(pix && vc_mipi_csi_dev)
      memcpy(pix, &vc_mipi_csi_dev->pix, sizeof(struct v4l2_pix_format));

    return;
}
EXPORT_SYMBOL(mx6s_get_pix_fmt);

/****** mx6s_set_pix_fmt = Set pixel format = 12.2020 *******************/
void mx6s_set_pix_fmt(struct v4l2_pix_format *pix)
{
    if(pix && vc_mipi_csi_dev)
    {
//      memcpy(&vc_mipi_csi_dev->pix, pix, sizeof(struct v4l2_pix_format));
      vc_mipi_csi_dev->pix.width        = pix->width;
      vc_mipi_csi_dev->pix.height       = pix->height;
      vc_mipi_csi_dev->pix.sizeimage    = pix->sizeimage;
      vc_mipi_csi_dev->pix.field        = pix->field;
      vc_mipi_csi_dev->pix.bytesperline = pix->bytesperline;
      vc_mipi_csi_dev->pix.pixelformat  = pix->pixelformat;
//      vc_mipi_csi_dev->type              = f->type;
    }

    return;
}
EXPORT_SYMBOL(mx6s_set_pix_fmt);

#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD2   // [[[
/****** mx6s_set_ctrl_range = Set user control range = 02.2021 **********/
int mx6s_set_ctrl_range (
    int id,             /* [in] control id                              */
    int minimum,        /* [in] minimum control value                   */
    int maximum,        /* [in] minimum control value                   */
    int default_value,  /* [in] default control value                   */
    int step )          /* [in] control's step value                    */
{
    struct mx6s_csi_dev *csi = vc_mipi_csi_dev;

    int num_ctrls = ARRAY_SIZE(ctrl_config_list);
    int ret = -2;
    int i;

    if(!csi) return -1;

    if(num_ctrls > MAX_NUM_CTRLS)
    {
      num_ctrls = MAX_NUM_CTRLS;
      dev_err(csi->dev, "%s: Number of user controls truncated to %d\n", __func__, num_ctrls);
    }

    for(i=0; i<num_ctrls; i++)
    {
      if(csi->ctrls[i]->id == id)
      {
        csi->ctrls[i]->minimum       = minimum;
        csi->ctrls[i]->maximum       = maximum;
        csi->ctrls[i]->default_value = default_value;
        csi->ctrls[i]->step          = step;
        ret = 0;
        break;
      }
    }

/**
 * struct v4l2_ctrl - The control structure.
 *
 * @node:   The list node.
 * @ev_subs:    The list of control event subscriptions.
 * @handler:    The handler that owns the control.
 * @cluster:    Point to start of cluster array.
 * @ncontrols:  Number of controls in cluster array.
 * @done:   Internal flag: set for each processed control.
 * @is_new: Set when the user specified a new value for this control. It
 *      is also set when called from v4l2_ctrl_handler_setup(). Drivers
 *      should never set this flag.
 * @has_changed: Set when the current value differs from the new value. Drivers
 *      should never use this flag.
 * @is_private: If set, then this control is private to its handler and it
 *      will not be added to any other handlers. Drivers can set
 *      this flag.
 * @is_auto:   If set, then this control selects whether the other cluster
 *      members are in 'automatic' mode or 'manual' mode. This is
 *      used for autogain/gain type clusters. Drivers should never
 *      set this flag directly.
 * @is_int:    If set, then this control has a simple integer value (i.e. it
 *      uses ctrl->val).
 * @is_string: If set, then this control has type %V4L2_CTRL_TYPE_STRING.
 * @is_ptr: If set, then this control is an array and/or has type >=
 *      %V4L2_CTRL_COMPOUND_TYPES
 *      and/or has type %V4L2_CTRL_TYPE_STRING. In other words, &struct
 *      v4l2_ext_control uses field p to point to the data.
 * @is_array: If set, then this control contains an N-dimensional array.
 * @has_volatiles: If set, then one or more members of the cluster are volatile.
 *      Drivers should never touch this flag.
 * @call_notify: If set, then call the handler's notify function whenever the
 *      control's value changes.
 * @manual_mode_value: If the is_auto flag is set, then this is the value
 *      of the auto control that determines if that control is in
 *      manual mode. So if the value of the auto control equals this
 *      value, then the whole cluster is in manual mode. Drivers should
 *      never set this flag directly.
 * @ops:    The control ops.
 * @type_ops:   The control type ops.
 * @id: The control ID.
 * @name:   The control name.
 * @type:   The control type.
 * @minimum:    The control's minimum value.
 * @maximum:    The control's maximum value.
 * @default_value: The control's default value.
 * @step:   The control's step value for non-menu controls.
 * @elems:  The number of elements in the N-dimensional array.
 * @elem_size:  The size in bytes of the control.
 * @dims:   The size of each dimension.
 * @nr_of_dims:The number of dimensions in @dims.
 * @menu_skip_mask: The control's skip mask for menu controls. This makes it
 *      easy to skip menu items that are not valid. If bit X is set,
 *      then menu item X is skipped. Of course, this only works for
 *      menus with <= 32 menu items. There are no menus that come
 *      close to that number, so this is OK. Should we ever need more,
 *      then this will have to be extended to a u64 or a bit array.
 * @qmenu:  A const char * array for all menu items. Array entries that are
 *      empty strings ("") correspond to non-existing menu items (this
 *      is in addition to the menu_skip_mask above). The last entry
 *      must be NULL.
 *      Used only if the @type is %V4L2_CTRL_TYPE_MENU.
 * @qmenu_int:  A 64-bit integer array for with integer menu items.
 *      The size of array must be equal to the menu size, e. g.:
 *      :math:`ceil(\frac{maximum - minimum}{step}) + 1`.
 *      Used only if the @type is %V4L2_CTRL_TYPE_INTEGER_MENU.
 * @flags:  The control's flags.
 * @cur:    Structure to store the current value.
 * @cur.val:    The control's current value, if the @type is represented via
 *      a u32 integer (see &enum v4l2_ctrl_type).
 * @val:    The control's new s32 value.
 * @priv:   The control's private pointer. For use by the driver. It is
 *      untouched by the control framework. Note that this pointer is
 *      not freed when the control is deleted. Should this be needed
 *      then a new internal bitfield can be added to tell the framework
 *      to free this pointer.
 * @p_cur:  The control's current value represented via a union which
 *      provides a standard way of accessing control types
 *      through a pointer.
 * @p_new:  The control's new value represented via a union which provides
 *      a standard way of accessing control types
 *      through a pointer.
 */

//struct v4l2_ctrl {
//    /* Administrative fields */
//    struct list_head node;
//    struct list_head ev_subs;
//    struct v4l2_ctrl_handler *handler;
//    struct v4l2_ctrl **cluster;
//    unsigned int ncontrols;
//
//    unsigned int done:1;
//
//    unsigned int is_new:1;
//    unsigned int has_changed:1;
//    unsigned int is_private:1;
//    unsigned int is_auto:1;
//    unsigned int is_int:1;
//    unsigned int is_string:1;
//    unsigned int is_ptr:1;
//    unsigned int is_array:1;
//    unsigned int has_volatiles:1;
//    unsigned int call_notify:1;
//    unsigned int manual_mode_value:8;
//
//    const struct v4l2_ctrl_ops *ops;
//    const struct v4l2_ctrl_type_ops *type_ops;
//    u32 id;
//    const char *name;
//    enum v4l2_ctrl_type type;
//    s64 minimum, maximum, default_value;
//    u32 elems;
//    u32 elem_size;
//    u32 dims[V4L2_CTRL_MAX_DIMS];
//    u32 nr_of_dims;
//    union {
//        u64 step;
//        u64 menu_skip_mask;
//    };
//    union {
//        const char * const *qmenu;
//        const s64 *qmenu_int;
//    };
//    unsigned long flags;
//    void *priv;
//    s32 val;
//    struct {
//        s32 val;
//    } cur;
//
//    union v4l2_ctrl_ptr p_new;
//    union v4l2_ctrl_ptr p_cur;
//};

    return ret;
}
EXPORT_SYMBOL(mx6s_set_ctrl_range);

#endif  // ]]]


#endif  /* ]]] */


static const struct of_device_id mx6s_csi_dt_ids[];

static inline int csi_read(struct mx6s_csi_dev *csi, unsigned int offset)
{
    return __raw_readl(csi->regbase + offset);
}
static inline void csi_write(struct mx6s_csi_dev *csi, unsigned int value,
                 unsigned int offset)
{
    __raw_writel(value, csi->regbase + offset);
}

static inline struct mx6s_csi_dev
                *notifier_to_mx6s_dev(struct v4l2_async_notifier *n)
{
    return container_of(n, struct mx6s_csi_dev, subdev_notifier);
}

struct mx6s_fmt *format_by_fourcc(int fourcc)
{
#define TRACE_FORMAT_BY_FOURCC  0   /* DDD - format_by_fourcc - trace */

    int i;

    for (i = 0; i < NUM_FORMATS; i++) {
        if (formats[i].pixelformat == fourcc)
        {
#if TRACE_FORMAT_BY_FOURCC
            pr_err("%s: pixelformat:'%4.4s'\n", __func__, (char *)&fourcc);
#endif
            return formats + i;
        }
    }

    pr_err("unknown pixelformat:'%4.4s'\n", (char *)&fourcc);
    return NULL;
}

struct mx6s_fmt *format_by_mbus(u32 code)
{
    int i;

    for (i = 0; i < NUM_FORMATS; i++) {
        if (formats[i].mbus_code == code)
            return formats + i;
    }

    pr_err("unknown mbus:0x%x\n", code);
    return NULL;
}

static struct mx6s_buffer *mx6s_ibuf_to_buf(struct mx6s_buf_internal *int_buf)
{
    return container_of(int_buf, struct mx6s_buffer, internal);
}

void csi_clk_enable(struct mx6s_csi_dev *csi_dev)
{
    clk_prepare_enable(csi_dev->clk_disp_axi);
    clk_prepare_enable(csi_dev->clk_disp_dcic);
    clk_prepare_enable(csi_dev->clk_csi_mclk);
}

void csi_clk_disable(struct mx6s_csi_dev *csi_dev)
{
    clk_disable_unprepare(csi_dev->clk_csi_mclk);
    clk_disable_unprepare(csi_dev->clk_disp_dcic);
    clk_disable_unprepare(csi_dev->clk_disp_axi);
}

static void csihw_reset(struct mx6s_csi_dev *csi_dev)
{
    __raw_writel(__raw_readl(csi_dev->regbase + CSI_CSICR3)
            | BIT_FRMCNT_RST,
            csi_dev->regbase + CSI_CSICR3);

    __raw_writel(CSICR1_RESET_VAL, csi_dev->regbase + CSI_CSICR1);
    __raw_writel(CSICR2_RESET_VAL, csi_dev->regbase + CSI_CSICR2);
    __raw_writel(CSICR3_RESET_VAL, csi_dev->regbase + CSI_CSICR3);
}

static void csisw_reset(struct mx6s_csi_dev *csi_dev)
{
    int cr1, cr3, cr18, isr;

    /* Disable csi  */
    cr18 = csi_read(csi_dev, CSI_CSICR18);
    cr18 &= ~BIT_CSI_ENABLE;
    csi_write(csi_dev, cr18, CSI_CSICR18);

    /* Clear RX FIFO */
    cr1 = csi_read(csi_dev, CSI_CSICR1);
    csi_write(csi_dev, cr1 & ~BIT_FCC, CSI_CSICR1);
    cr1 = csi_read(csi_dev, CSI_CSICR1);
    csi_write(csi_dev, cr1 | BIT_CLR_RXFIFO, CSI_CSICR1);

    /* DMA reflash */
    cr3 = csi_read(csi_dev, CSI_CSICR3);
    cr3 |= BIT_DMA_REFLASH_RFF | BIT_FRMCNT_RST;
    csi_write(csi_dev, cr3, CSI_CSICR3);

    msleep(2);

    cr1 = csi_read(csi_dev, CSI_CSICR1);
    csi_write(csi_dev, cr1 | BIT_FCC, CSI_CSICR1);

    isr = csi_read(csi_dev, CSI_CSISR);
    csi_write(csi_dev, isr, CSI_CSISR);

    cr18 |= csi_dev->soc->baseaddr_switch;

    /* Enable csi  */
    cr18 |= BIT_CSI_ENABLE;
    csi_write(csi_dev, cr18, CSI_CSICR18);
}

/*!
 * csi_init_interface
 *    Init csi interface
 */
static void csi_init_interface(struct mx6s_csi_dev *csi_dev)
{
    unsigned int val = 0;
    unsigned int imag_para;

    val |= BIT_SOF_POL;
    val |= BIT_REDGE;
    val |= BIT_GCLK_MODE;
    val |= BIT_HSYNC_POL;
    val |= BIT_FCC;
    val |= 1 << SHIFT_MCLKDIV;
    val |= BIT_MCLKEN;
    __raw_writel(val, csi_dev->regbase + CSI_CSICR1);

    imag_para = (640 << 16) | 960;
    __raw_writel(imag_para, csi_dev->regbase + CSI_CSIIMAG_PARA);

    val = BIT_DMA_REFLASH_RFF;
    __raw_writel(val, csi_dev->regbase + CSI_CSICR3);
}

static void csi_enable_int(struct mx6s_csi_dev *csi_dev, int arg)
{
    unsigned long cr1 = __raw_readl(csi_dev->regbase + CSI_CSICR1);

    cr1 |= BIT_SOF_INTEN;
    cr1 |= BIT_RFF_OR_INT;
    if (arg == 1) {
        /* still capture needs DMA intterrupt */
        cr1 |= BIT_FB1_DMA_DONE_INTEN;
        cr1 |= BIT_FB2_DMA_DONE_INTEN;
    }
    __raw_writel(cr1, csi_dev->regbase + CSI_CSICR1);
}

static void csi_disable_int(struct mx6s_csi_dev *csi_dev)
{
    unsigned long cr1 = __raw_readl(csi_dev->regbase + CSI_CSICR1);

    cr1 &= ~BIT_SOF_INTEN;
    cr1 &= ~BIT_RFF_OR_INT;
    cr1 &= ~BIT_FB1_DMA_DONE_INTEN;
    cr1 &= ~BIT_FB2_DMA_DONE_INTEN;
    __raw_writel(cr1, csi_dev->regbase + CSI_CSICR1);
}

static void csi_enable(struct mx6s_csi_dev *csi_dev, int arg)
{
    unsigned long cr = __raw_readl(csi_dev->regbase + CSI_CSICR18);

    if (arg == 1)
        cr |= BIT_CSI_ENABLE;
    else
        cr &= ~BIT_CSI_ENABLE;
    __raw_writel(cr, csi_dev->regbase + CSI_CSICR18);
}

static void csi_buf_stride_set(struct mx6s_csi_dev *csi_dev, u32 stride)
{
    __raw_writel(stride, csi_dev->regbase + CSI_CSIFBUF_PARA);
}

static void csi_deinterlace_enable(struct mx6s_csi_dev *csi_dev, bool enable)
{
    unsigned long cr18 = __raw_readl(csi_dev->regbase + CSI_CSICR18);

    if (enable == true)
        cr18 |= BIT_DEINTERLACE_EN;
    else
        cr18 &= ~BIT_DEINTERLACE_EN;

    __raw_writel(cr18, csi_dev->regbase + CSI_CSICR18);
}

static void csi_deinterlace_mode(struct mx6s_csi_dev *csi_dev, int mode)
{
    unsigned long cr18 = __raw_readl(csi_dev->regbase + CSI_CSICR18);

    if (mode == V4L2_STD_NTSC)
        cr18 |= BIT_NTSC_EN;
    else
        cr18 &= ~BIT_NTSC_EN;

    __raw_writel(cr18, csi_dev->regbase + CSI_CSICR18);
}

static void csi_tvdec_enable(struct mx6s_csi_dev *csi_dev, bool enable)
{

#define TRACE_CSI_TVDEC_ENABLE  0   /* DDD - csi_tvdec_enable - trace */

    unsigned long cr18 = __raw_readl(csi_dev->regbase + CSI_CSICR18);
    unsigned long cr1 = __raw_readl(csi_dev->regbase + CSI_CSICR1);

#if TRACE_CSI_TVDEC_ENABLE
        dev_err(csi_dev->dev,"%s: enable=%d (0=false,1=true)\n", __func__, (int)enable);
#endif

    if (enable == true) {
        cr18 |= (BIT_TVDECODER_IN_EN |
                BIT_BASEADDR_SWITCH_EN |
                BIT_BASEADDR_SWITCH_SEL |
                BIT_BASEADDR_CHG_ERR_EN);
        cr1 |= BIT_CCIR_MODE;
        cr1 &= ~(BIT_SOF_POL | BIT_REDGE);
    } else {
        cr18 &= ~(BIT_TVDECODER_IN_EN |
                BIT_BASEADDR_SWITCH_EN |
                BIT_BASEADDR_SWITCH_SEL |
                BIT_BASEADDR_CHG_ERR_EN);
        cr1 &= ~BIT_CCIR_MODE;
        cr1 |= BIT_SOF_POL | BIT_REDGE;
    }

    __raw_writel(cr18, csi_dev->regbase + CSI_CSICR18);
    __raw_writel(cr1, csi_dev->regbase + CSI_CSICR1);
}

/****** csi_dmareq_rff_enable = DMA rff enable = 11.2020 ****************/
static void csi_dmareq_rff_enable(struct mx6s_csi_dev *csi_dev)
{

#define TRACE_CSI_DMAREQ_RFF_ENABLE     0   /* DDD - csi_dmareq_rff_enable - trace */

    unsigned long cr3 = __raw_readl(csi_dev->regbase + CSI_CSICR3);
    unsigned long cr2 = __raw_readl(csi_dev->regbase + CSI_CSICR2);

    /* Burst Type of DMA Transfer from RxFIFO. INCR16 */
    cr2 |= 0xC0000000;

    cr3 |= BIT_DMA_REQ_EN_RFF;
    cr3 |= BIT_HRESP_ERR_EN;
    cr3 &= ~BIT_RXFF_LEVEL;
    cr3 |= 0x2 << 4;
    if (csi_dev->csi_two_8bit_sensor_mode)
    {
#if TRACE_CSI_DMAREQ_RFF_ENABLE
        dev_err(csi_dev->dev,"%s: csi_two_8bit_sensor_mode=true\n", __func__);
#endif
        cr3 |= BIT_TWO_8BIT_SENSOR;
    }
#if VC_MIPI
    else
    {
#if TRACE_CSI_DMAREQ_RFF_ENABLE
        dev_err(csi_dev->dev,"%s: csi_two_8bit_sensor_mode=false\n", __func__);
#endif
        cr3 &= ~BIT_TWO_8BIT_SENSOR;
    }
#endif

#if TRACE_CSI_DMAREQ_RFF_ENABLE
    dev_err(csi_dev->dev,"%s: CSI_CSICR3=0x%08x\n", __func__, (int)cr3);
#endif

    __raw_writel(cr3, csi_dev->regbase + CSI_CSICR3);
    __raw_writel(cr2, csi_dev->regbase + CSI_CSICR2);
}

static void csi_dmareq_rff_disable(struct mx6s_csi_dev *csi_dev)
{
    unsigned long cr3 = __raw_readl(csi_dev->regbase + CSI_CSICR3);

    cr3 &= ~BIT_DMA_REQ_EN_RFF;
    cr3 &= ~BIT_HRESP_ERR_EN;
    __raw_writel(cr3, csi_dev->regbase + CSI_CSICR3);
}

static void csi_set_imagpara(struct mx6s_csi_dev *csi,
                    int width, int height)
{
    int imag_para = 0;
    unsigned long cr3 = __raw_readl(csi->regbase + CSI_CSICR3);

    imag_para = (width << 16) | height;
    __raw_writel(imag_para, csi->regbase + CSI_CSIIMAG_PARA);

    /* reflash the embeded DMA controller */
    __raw_writel(cr3 | BIT_DMA_REFLASH_RFF, csi->regbase + CSI_CSICR3);
}

/****** csi_error_recovery = CSI error recovery = 01.2021 ***************/
static void csi_error_recovery(struct mx6s_csi_dev *csi_dev)
{

#define TRACE_CSI_ERROR_RECOVERY    0   /* DDD - csi_error_recovery - trace */

    u32 cr1, cr3, cr18;
    /* software reset */

#if TRACE_CSI_ERROR_RECOVERY
    dev_err(csi_dev->dev, "%s: \n", __func__);
#endif

    /* Disable csi  */
    cr18 = csi_read(csi_dev, CSI_CSICR18);
    cr18 &= ~BIT_CSI_ENABLE;
    csi_write(csi_dev, cr18, CSI_CSICR18);

    /* Clear RX FIFO */
    cr1 = csi_read(csi_dev, CSI_CSICR1);
    csi_write(csi_dev, cr1 & ~BIT_FCC, CSI_CSICR1);
    cr1 = csi_read(csi_dev, CSI_CSICR1);
    csi_write(csi_dev, cr1 | BIT_CLR_RXFIFO, CSI_CSICR1);

    cr1 = csi_read(csi_dev, CSI_CSICR1);
    csi_write(csi_dev, cr1 | BIT_FCC, CSI_CSICR1);

    /* DMA reflash */
    cr3 = csi_read(csi_dev, CSI_CSICR3);
    cr3 |= BIT_DMA_REFLASH_RFF;
    csi_write(csi_dev, cr3, CSI_CSICR3);

    /* Ensable csi  */
    cr18 |= BIT_CSI_ENABLE;
    csi_write(csi_dev, cr18, CSI_CSICR18);
}

#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD2  // [[[

/****** mx6s_set_ctrl = Set control = 01.2021 ***************************/
static int mx6s_set_ctrl(struct v4l2_ctrl *ctrl)
{
    struct mx6s_csi_dev *csi_dev =
        container_of(ctrl->handler, struct mx6s_csi_dev, ctrl_handler);

    struct v4l2_control c;

    c.id = ctrl->id;
    c.value = ctrl->val;

//    if (ctrl->id == V4L2_CID_EXPOSURE_ABSOLUTE) {
//        c.id = V4L2_CID_EXPOSURE;
//    }

    return v4l2_s_ctrl(NULL, csi_dev->sd->ctrl_handler, &c);
}

static const struct v4l2_ctrl_ops mx6s_ctrl_ops = {
    .s_ctrl = mx6s_set_ctrl,
};

/* Initialize control handlers */
/****** mx6s_init_controls = Init controls = 01.2021 ********************/
static int mx6s_init_controls(struct mx6s_csi_dev *csi)
{
    struct v4l2_ctrl_handler *ctrl_hdlr;
//    struct v4l2_ctrl *ctrls[10];
    int num_ctrls = ARRAY_SIZE(ctrl_config_list);
    int ret;
    int i;

    if(num_ctrls > MAX_NUM_CTRLS)
    {
      num_ctrls = MAX_NUM_CTRLS;
      dev_err(csi->dev, "%s: Number of user controls truncated to %d\n", __func__, num_ctrls);
    }

    ctrl_hdlr = &csi->ctrl_handler;
    ret = v4l2_ctrl_handler_init(ctrl_hdlr, num_ctrls);
    if (ret)
        return ret;

// Moved to vc_mipi_user_ctrl.h to be shared with IMX driver.
//    static struct v4l2_ctrl_config ctrl_config_list[] =
//    {
//    /* Do not change the name field for the controls! */
//      {
//          .ops = &mx6s_ctrl_ops,
//          .id = V4L2_CID_GAIN,
//          .name = "Gain",
//          .type = V4L2_CTRL_TYPE_INTEGER,
//          .flags = V4L2_CTRL_FLAG_SLIDER,
//          .min = 0,
//          .max = 0xfff,
//          .def = 10,
//          .step = 1,
//      },
//      {
//          .ops = &mx6s_ctrl_ops,
//          .id = V4L2_CID_EXPOSURE,
//          .name = "Exposure",
//          .type = V4L2_CTRL_TYPE_INTEGER,
//          .flags = V4L2_CTRL_FLAG_SLIDER,
//          .min = 0,
//          .max = 0xfffff,
//          .def = 1000,
//          .step = 1,
//      },
//    };

    for (i = 0; i < num_ctrls; i++)
    {
        ctrl_config_list[i].ops = &mx6s_ctrl_ops;
        csi->ctrls[i] = v4l2_ctrl_new_custom(ctrl_hdlr, &ctrl_config_list[i], NULL);
        if (csi->ctrls[i] == NULL)
        {
            dev_err(csi->dev, "Failed to init %s ctrl\n", ctrl_config_list[i].name);
            continue;
        }

//        if (ctrl_config_list[i].type == V4L2_CTRL_TYPE_STRING &&
//            ctrl_config_list[i].flags & V4L2_CTRL_FLAG_READ_ONLY) {
//            ctrl->p_new.p_char = devm_kzalloc(&client->dev,
//                ctrl_config_list[i].max + 1, GFP_KERNEL);
//            if (!ctrl->p_new.p_char)
//                return -ENOMEM;
//        }
//        priv->ctrls[i] = ctrl;
    }

#define CHANGE_GAIN_RANGE   0   /* DDD - change gain range test */
#if CHANGE_GAIN_RANGE
    csi->ctrls[0]->minimum = 3;
    csi->ctrls[0]->maximum = 4444;
    csi->ctrls[0]->default_value = 34;
    csi->ctrls[0]->step = 2;
#endif

//    v4l2_ctrl_new_std(ctrl_hdlr, &mx6s_ctrl_ops, V4L2_CID_ANALOGUE_GAIN,
//              0, 888,
//              1,  0);
//
//    v4l2_ctrl_new_std(ctrl_hdlr, &mx6s_ctrl_ops, V4L2_CID_DIGITAL_GAIN,
//              0, 999,
//              1, 0);
//
//    v4l2_ctrl_new_std(ctrl_hdlr, &mx6s_ctrl_ops, V4L2_CID_EXPOSURE,
//              0, 0xffff, 4, 1000);
//
//    v4l2_ctrl_new_std(ctrl_hdlr, &mx6s_ctrl_ops, V4L2_CID_EXPOSURE_ABSOLUTE,
//              0, 0xffff, 4, 1000);
//
//    v4l2_ctrl_new_std(ctrl_hdlr, &mx6s_ctrl_ops, V4L2_CID_WIDE_DYNAMIC_RANGE,
//              0, 1, 1, 0);

    /* Really not sure about locking... enabling it blindly causes dead locks */
    /* ctrl_hdlr->lock = &csi->lock; */

    csi->v4l2_dev.ctrl_handler = ctrl_hdlr;

    return 0;
}


static void mx6s_free_controls(struct mx6s_csi_dev *csi)
{
    v4l2_ctrl_handler_free(&csi->ctrl_handler);
}

#endif  // ]]]

/*
 *  Videobuf operations
 */
static int mx6s_videobuf_setup(struct vb2_queue *vq,
            unsigned int *count, unsigned int *num_planes,
            unsigned int sizes[], struct device *alloc_devs[])
{

#define TRACE_MX6S_VIDEOBUF_SETUP   0   /* DDD - mx6s_videobuf_setup - trace */

    struct mx6s_csi_dev *csi_dev = vb2_get_drv_priv(vq);

#if TRACE_MX6S_VIDEOBUF_SETUP
    dev_err(csi_dev->dev, "%s: count=%d, size=%d\n", __func__, *count, sizes[0]);
#endif

    alloc_devs[0] = csi_dev->dev;

    sizes[0] = csi_dev->pix.sizeimage;

    pr_debug("%s: size=%d\n", __func__, sizes[0]);
    if (0 == *count)
        *count = 32;
    if (!*num_planes &&
        sizes[0] * *count > MAX_VIDEO_MEM * 1024 * 1024)
        *count = (MAX_VIDEO_MEM * 1024 * 1024) / sizes[0];

#if TRACE_MX6S_VIDEOBUF_SETUP
    dev_err(csi_dev->dev, "%s: size=%d, count=%d\n", __func__, sizes[0], *count);
#endif

    *num_planes = 1;

    return 0;
}

static int mx6s_videobuf_prepare(struct vb2_buffer *vb)
{
    struct mx6s_csi_dev *csi_dev = vb2_get_drv_priv(vb->vb2_queue);
    int ret = 0;

    dev_dbg(csi_dev->dev, "%s (vb=0x%p) 0x%p %lu\n", __func__,
        vb, vb2_plane_vaddr(vb, 0), vb2_get_plane_payload(vb, 0));

#ifdef DEBUG
    /*
     * This can be useful if you want to see if we actually fill
     * the buffer with something
     */
    if (vb2_plane_vaddr(vb, 0))
        memset((void *)vb2_plane_vaddr(vb, 0),
               0xaa, vb2_get_plane_payload(vb, 0));
#endif

    vb2_set_plane_payload(vb, 0, csi_dev->pix.sizeimage);
    if (vb2_plane_vaddr(vb, 0) &&
        vb2_get_plane_payload(vb, 0) > vb2_plane_size(vb, 0)) {
        ret = -EINVAL;
        goto out;
    }

    return 0;

out:
    return ret;
}

static void mx6s_videobuf_queue(struct vb2_buffer *vb)
{
    struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
    struct mx6s_csi_dev *csi_dev = vb2_get_drv_priv(vb->vb2_queue);
    struct mx6s_buffer *buf = container_of(vbuf, struct mx6s_buffer, vb);
    unsigned long flags;

    dev_dbg(csi_dev->dev, "%s (vb=0x%p) 0x%p %lu\n", __func__,
        vb, vb2_plane_vaddr(vb, 0), vb2_get_plane_payload(vb, 0));

    spin_lock_irqsave(&csi_dev->slock, flags);

    list_add_tail(&buf->internal.queue, &csi_dev->capture);

    spin_unlock_irqrestore(&csi_dev->slock, flags);
}

static void mx6s_update_csi_buf(struct mx6s_csi_dev *csi_dev,
                 unsigned long phys, int bufnum)
{
    if (bufnum == 1)
        csi_write(csi_dev, phys, CSI_CSIDMASA_FB2);
    else
        csi_write(csi_dev, phys, CSI_CSIDMASA_FB1);
}

static void mx6s_csi_init(struct mx6s_csi_dev *csi_dev)
{
    csi_clk_enable(csi_dev);
    csihw_reset(csi_dev);
    csi_init_interface(csi_dev);
    csi_dmareq_rff_disable(csi_dev);
}

static void mx6s_csi_deinit(struct mx6s_csi_dev *csi_dev)
{
    csihw_reset(csi_dev);
    csi_init_interface(csi_dev);
    csi_dmareq_rff_disable(csi_dev);
    csi_clk_disable(csi_dev);
}

/****** mx6s_csi_enable = CSI enable = 01.2021 **************************/
static int mx6s_csi_enable(struct mx6s_csi_dev *csi_dev)
{

#define TRACE_MX6S_CSI_ENABLE       0   /* DDD - mx6s_csi_enable - trace */

#define MX6S_CSI_ENABLE_TVDEC_DISABLE   0   /* CCC - mx6s_csi_enable - disable tvdec mode             */
                                            /*       PK 06.01.2021: Don't set this macro to 1 !!      */
                                            /*       It enables occasional shifted up/left bad frames */

#define TIMEOUT                  10000000   /* CCC - mx6s_csi_enable - timeout 1 */
#define TIMEOUT2                  1000000   /* CCC - mx6s_csi_enable - timeout 2 */

    struct v4l2_pix_format *pix = &csi_dev->pix;
    unsigned long flags;
    unsigned long val;
    int timeout, timeout2;

    csi_dev->skipframe = 0;
    csisw_reset(csi_dev);

#if TRACE_MX6S_CSI_ENABLE
    dev_err(csi_dev->dev, "%s: pix->field=%d\n", __func__, pix->field);
#endif

    if (pix->field == V4L2_FIELD_INTERLACED)
    {
#if TRACE_MX6S_CSI_ENABLE
        dev_err(csi_dev->dev, "%s: pix->field=%d: V4L2_FIELD_INTERLACED\n", __func__, pix->field);
#endif
        csi_tvdec_enable(csi_dev, true);
    }

#if VC_MIPI
#if MX6S_CSI_ENABLE_TVDEC_DISABLE // disable tvdec mode
    else
    {
        csi_tvdec_enable(csi_dev, false);
    }
#endif
#endif

    /* For mipi csi input only */
    if (csi_dev->csi_mipi_mode == true) {
        csi_dmareq_rff_enable(csi_dev);
        csi_enable_int(csi_dev, 1);
        csi_enable(csi_dev, 1);
        return 0;
    }

    local_irq_save(flags);
    for (timeout = TIMEOUT; timeout > 0; timeout--) {
        if (csi_read(csi_dev, CSI_CSISR) & BIT_SOF_INT) {
            val = csi_read(csi_dev, CSI_CSICR3);
            csi_write(csi_dev, val | BIT_DMA_REFLASH_RFF,
                    CSI_CSICR3);
            /* Wait DMA reflash done */
            for (timeout2 = TIMEOUT2; timeout2 > 0; timeout2--) {
                if (csi_read(csi_dev, CSI_CSICR3) &
                    BIT_DMA_REFLASH_RFF)
                    cpu_relax();
                else
                    break;
            }
            if (timeout2 <= 0) {
                pr_err("timeout when wait for reflash done.\n");
                local_irq_restore(flags);
                return -ETIME;
            }
            /* For imx6sl csi, DMA FIFO will auto start when sensor ready to work,
             * so DMA should enable right after FIFO reset, otherwise dma will lost data
             * and image will split.
             */
            csi_dmareq_rff_enable(csi_dev);
            csi_enable_int(csi_dev, 1);
            csi_enable(csi_dev, 1);
            break;
        } else
            cpu_relax();
    }
    if (timeout <= 0) {
        pr_err("timeout when wait for SOF\n");
        local_irq_restore(flags);
        return -ETIME;
    }
    local_irq_restore(flags);

#if TRACE_MX6S_CSI_ENABLE
    dev_err(csi_dev->dev, "%s: Exit\n", __func__);
#endif

    return 0;
}

static void mx6s_csi_disable(struct mx6s_csi_dev *csi_dev)
{
    struct v4l2_pix_format *pix = &csi_dev->pix;

    csi_dmareq_rff_disable(csi_dev);
    csi_disable_int(csi_dev);

    /* set CSI_CSIDMASA_FB1 and CSI_CSIDMASA_FB2 to default value */
    csi_write(csi_dev, 0, CSI_CSIDMASA_FB1);
    csi_write(csi_dev, 0, CSI_CSIDMASA_FB2);

    csi_buf_stride_set(csi_dev, 0);

    if (pix->field == V4L2_FIELD_INTERLACED) {
        csi_deinterlace_enable(csi_dev, false);
        csi_tvdec_enable(csi_dev, false);
    }

    csi_enable(csi_dev, 0);
}

/****** mx6s_configure_csi = Configure CSI = 11.2020 ********************/
static int mx6s_configure_csi(struct mx6s_csi_dev *csi_dev)
{

#define TRACE_MX6S_CONFIGURE_CSI    0   /* DDD - mx6s_configure_csi - trace */
#define DUMP_MIPI_CSI_ISP_CONFIG    0   /* DDD - mx6s_configure_csi - dump MIPI_CSI_ISP_CONFIG registers */

    struct v4l2_pix_format *pix = &csi_dev->pix;
    u32 cr1, cr18;
    u32 width;

    if (pix->field == V4L2_FIELD_INTERLACED) {
        csi_deinterlace_enable(csi_dev, true);
        csi_buf_stride_set(csi_dev, csi_dev->pix.width);
        csi_deinterlace_mode(csi_dev, csi_dev->std);
    } else {
        csi_deinterlace_enable(csi_dev, false);
        csi_buf_stride_set(csi_dev, 0);
    }

    switch (csi_dev->fmt->pixelformat) {
    case V4L2_PIX_FMT_YUV32:
    case V4L2_PIX_FMT_SBGGR8:
        width = pix->width;
        break;

    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_YUYV:
        if (csi_dev->csi_mipi_mode == true)
            width = pix->width;
        else
            /* For parallel 8-bit sensor input */
            width = pix->width * 2;
        break;

#if VC_MIPI
    case V4L2_PIX_FMT_GREY:
    case V4L2_PIX_FMT_SRGGB8:
        width = pix->width;
        csi_dev->csi_two_8bit_sensor_mode = false;
        break;

    case V4L2_PIX_FMT_Y10:
    case V4L2_PIX_FMT_SRGGB10:
    case V4L2_PIX_FMT_SGBRG10:
        width = pix->width;
        csi_dev->csi_two_8bit_sensor_mode = true;
//        width = pix->width * 2;
//         width = 16;
//         width = 32;
//        width = pix->width - 16;
//        dev_err(csi_dev->dev, "------ %s: 10-bit: width=%d\n", __func__, width);
        break;

    case V4L2_PIX_FMT_Y12:
    case V4L2_PIX_FMT_SRGGB12:
        width = pix->width;
        csi_dev->csi_two_8bit_sensor_mode = true;
        break;
#endif

    default:
        dev_err(csi_dev->dev,"%s: case not supported\n", __func__);
        return -EINVAL;
    }

#if TRACE_MX6S_CONFIGURE_CSI
{
    int pix_fmt = csi_dev->fmt->pixelformat;
    dev_err(csi_dev->dev, "%s: width=%d pixelformat=0x%08x '%c%c%c%c'\n", __func__,
                        width,
                        pix_fmt,
                        (char)((pix_fmt      ) & 0xFF),
                        (char)((pix_fmt >>  8) & 0xFF),
                        (char)((pix_fmt >> 16) & 0xFF),
                        (char)((pix_fmt >> 24) & 0xFF));
}
#endif

#if DUMP_MIPI_CSI_ISP_CONFIG
{
    int cr;

    cr = 0x2B << 2;
    dev_err(csi_dev->dev, "%s: Write MIPI_CSI_ISP_CONFIG0: val=0x%x\n", __func__, cr);
    csi_write(csi_dev, cr, MIPI_CSI_ISP_CONFIG0);
    cr = 0;

    cr = csi_read(csi_dev, MIPI_CSI_ISP_CONFIG0);
    dev_err(csi_dev->dev, "%s: MIPI_CSI_ISP_CONFIG0: pixel_mode=0x%x data_format=0x%x\n", __func__,
                (cr >> 12) & 0x3, (cr >> 2) & 0x3F);


//#if 0
//    cr = csi_read(csi_dev, MIPI_CSI_ISP_CONFIG1);
//    dev_err(csi_dev->dev, "%s: MIPI_CSI_ISP_CONFIG1: pixel_mode=0x%x data_format=0x%x\n", __func__,
//                (cr >> 12) & 0x3, (cr >> 2) & 0x3F);
//
//    cr = csi_read(csi_dev, MIPI_CSI_ISP_CONFIG2);
//    dev_err(csi_dev->dev, "%s: MIPI_CSI_ISP_CONFIG2: pixel_mode=0x%x data_format=0x%x\n", __func__,
//                (cr >> 12) & 0x3, (cr >> 2) & 0x3F);
//
//    cr = csi_read(csi_dev, MIPI_CSI_ISP_CONFIG3);
//    dev_err(csi_dev->dev, "%s: MIPI_CSI_ISP_CONFIG3: pixel_mode=0x%x data_format=0x%x\n", __func__,
//                (cr >> 12) & 0x3, (cr >> 2) & 0x3F);
//#endif

}
#endif

    csi_set_imagpara(csi_dev, width, pix->height);

    if (csi_dev->csi_mipi_mode == true) {
        cr1 = csi_read(csi_dev, CSI_CSICR1);
        cr1 &= ~BIT_GCLK_MODE;
        csi_write(csi_dev, cr1, CSI_CSICR1);

        cr18 = csi_read(csi_dev, CSI_CSICR18);
        cr18 &= ~BIT_MIPI_DATA_FORMAT_MASK;
        cr18 |= BIT_DATA_FROM_MIPI;


        switch (csi_dev->fmt->pixelformat) {
        case V4L2_PIX_FMT_UYVY:
        case V4L2_PIX_FMT_YUYV:
            cr18 |= BIT_MIPI_DATA_FORMAT_YUV422_8B;
            break;
        case V4L2_PIX_FMT_SBGGR8:
            cr18 |= BIT_MIPI_DATA_FORMAT_RAW8;
            break;

#if VC_MIPI
        case V4L2_PIX_FMT_GREY:
        case V4L2_PIX_FMT_SRGGB8:
#if TRACE_MX6S_CONFIGURE_CSI
            dev_err(csi_dev->dev, "%s: Set 8-bit raw format\n", __func__);
#endif
            cr18 |= BIT_MIPI_DATA_FORMAT_RAW8;
            break;
        case V4L2_PIX_FMT_Y10:
        case V4L2_PIX_FMT_SRGGB10:
        case V4L2_PIX_FMT_SGBRG10:
#if TRACE_MX6S_CONFIGURE_CSI
            dev_err(csi_dev->dev, "%s: Set 10-bit raw format\n", __func__);
#endif
            cr18 |= BIT_MIPI_DATA_FORMAT_RAW10;
            break;
        case V4L2_PIX_FMT_Y12:
        case V4L2_PIX_FMT_SRGGB12:
#if TRACE_MX6S_CONFIGURE_CSI
            dev_err(csi_dev->dev, "%s: Set 12-bit raw format\n", __func__);
#endif
            cr18 |= BIT_MIPI_DATA_FORMAT_RAW12;
            break;
#endif

        default:
            pr_debug("   fmt not supported\n");
            dev_err(csi_dev->dev,"%s: fmt not supported\n", __func__);
            return -EINVAL;
        }


        csi_write(csi_dev, cr18, CSI_CSICR18);

#if TRACE_MX6S_CONFIGURE_CSI
        cr18 = csi_read(csi_dev, CSI_CSICR18);
        dev_err(csi_dev->dev, "%s: CSI_CSICR18: 0x%08x data_format=0x%x\n", __func__,
                cr18, (cr18 >> 25) & 0x3F);
#endif


    }
    return 0;
}

/****** mx6s_start_streaming = Start streaming = 12.2020 ****************/
static int mx6s_start_streaming(struct vb2_queue *vq, unsigned int count)
{

#define TRACE_MX6S_START_STREAMING  0   /* DDD - mx6s_start_streaming - trace */
#define MX6S_START_STREAMING_DELAY  0   /* CCC - mx6s_start_streaming - time delay in ms: 0=off */

#define FRAME_BUF_ALLOC_METHOD      0   /* CCC - mx6s_start_streaming - frame buffer memory allocation/free method: */
                                        /*         0 : original dma_alloc_coherent/dma_free_coherent                */
                                        /*         1 : use kzalloc/kfree for memory allocation                      */
                                        /*         2 : use devm_kzalloc/kfree for memory allocation                 */

    struct mx6s_csi_dev *csi_dev = vb2_get_drv_priv(vq);
    struct vb2_buffer *vb;
    struct mx6s_buffer *buf;
    unsigned long phys;
    unsigned long flags;

#if TRACE_MX6S_START_STREAMING
    dev_err(csi_dev->dev, "%s: count=%d (>=2)\n", __func__, count);
#endif

#if MX6S_START_STREAMING_DELAY
    mdelay(MX6S_START_STREAMING_DELAY);
#endif

    if (count < 2)
        return -ENOBUFS;

    /*
     * I didn't manage to properly enable/disable
     * a per frame basis during running transfers,
     * thus we allocate a buffer here and use it to
     * discard frames when no buffer is available.
     * Feel free to work on this ;)
     */
    csi_dev->discard_size = csi_dev->pix.sizeimage;

#if TRACE_MX6S_START_STREAMING
//    csi_dev->discard_size = csi_dev->pix.sizeimage*2;       // ???
    dev_err(csi_dev->dev, "%s: discard_size=%d\n", __func__, (int)csi_dev->discard_size);
#endif

#if FRAME_BUF_ALLOC_METHOD == 0     // original method
    csi_dev->discard_buffer = dma_alloc_coherent(csi_dev->v4l2_dev.dev,
                    PAGE_ALIGN(csi_dev->discard_size),
                    &csi_dev->discard_buffer_dma,
                    GFP_DMA | GFP_KERNEL);
#else
// ???
    csi_dev->discard_buffer = kzalloc(PAGE_ALIGN(csi_dev->discard_size), GFP_DMA | GFP_KERNEL);
//    csi_dev->discard_buffer = kzalloc(PAGE_ALIGN(csi_dev->discard_size), GFP_KERNEL);
    csi_dev->discard_buffer_dma = (dma_addr_t) csi_dev->discard_buffer;
    dev_err(csi_dev->dev, "%s: kzalloc used, size=%d\n", __func__, (int)csi_dev->discard_size);
#endif
    if (!csi_dev->discard_buffer)
    {
        dev_err(csi_dev->dev, "%s: csi_dev->discard_buffer alloc error\n", __func__);
        return -ENOMEM;
    }

    spin_lock_irqsave(&csi_dev->slock, flags);

    csi_dev->buf_discard[0].discard = true;
    list_add_tail(&csi_dev->buf_discard[0].queue,
              &csi_dev->discard);

    csi_dev->buf_discard[1].discard = true;
    list_add_tail(&csi_dev->buf_discard[1].queue,
              &csi_dev->discard);

    /* csi buf 0 */
    buf = list_first_entry(&csi_dev->capture, struct mx6s_buffer,
                   internal.queue);
    buf->internal.bufnum = 0;
    vb = &buf->vb.vb2_buf;
    vb->state = VB2_BUF_STATE_ACTIVE;

    phys = vb2_dma_contig_plane_dma_addr(vb, 0);

    mx6s_update_csi_buf(csi_dev, phys, buf->internal.bufnum);
    list_move_tail(csi_dev->capture.next, &csi_dev->active_bufs);

    /* csi buf 1 */
    buf = list_first_entry(&csi_dev->capture, struct mx6s_buffer,
                   internal.queue);
    buf->internal.bufnum = 1;
    vb = &buf->vb.vb2_buf;
    vb->state = VB2_BUF_STATE_ACTIVE;

    phys = vb2_dma_contig_plane_dma_addr(vb, 0);
    mx6s_update_csi_buf(csi_dev, phys, buf->internal.bufnum);
    list_move_tail(csi_dev->capture.next, &csi_dev->active_bufs);

    csi_dev->nextfb = 0;

    spin_unlock_irqrestore(&csi_dev->slock, flags);

#if TRACE_MX6S_START_STREAMING
    dev_err(csi_dev->dev, "%s: Exit\n", __func__);
#endif
    return mx6s_csi_enable(csi_dev);
}

/****** mx6s_stop_streaming = Stop streaming = 01.2021 ******************/
static void mx6s_stop_streaming(struct vb2_queue *vq)
{

#define TRACE_MX6S_STOP_STREAMING   0   /* DDD - mx6s_stop_streaming - trace */

    struct mx6s_csi_dev *csi_dev = vb2_get_drv_priv(vq);
    unsigned long flags;
    struct mx6s_buffer *buf, *tmp;
    void *b;

#if TRACE_MX6S_STOP_STREAMING
    dev_err(csi_dev->dev, "%s:...\n", __func__);
#endif

    mx6s_csi_disable(csi_dev);

    spin_lock_irqsave(&csi_dev->slock, flags);

    list_for_each_entry_safe(buf, tmp,
                &csi_dev->active_bufs, internal.queue) {
        list_del_init(&buf->internal.queue);
        if (buf->vb.vb2_buf.state == VB2_BUF_STATE_ACTIVE)
            vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
    }

    list_for_each_entry_safe(buf, tmp,
                &csi_dev->capture, internal.queue) {
        list_del_init(&buf->internal.queue);
        if (buf->vb.vb2_buf.state == VB2_BUF_STATE_ACTIVE)
            vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
    }

    INIT_LIST_HEAD(&csi_dev->capture);
    INIT_LIST_HEAD(&csi_dev->active_bufs);
    INIT_LIST_HEAD(&csi_dev->discard);

    b = csi_dev->discard_buffer;
    csi_dev->discard_buffer = NULL;

    spin_unlock_irqrestore(&csi_dev->slock, flags);

#if FRAME_BUF_ALLOC_METHOD == 0     // original method
    dma_free_coherent(csi_dev->v4l2_dev.dev,
                csi_dev->discard_size, b,
                csi_dev->discard_buffer_dma);
#else
    if(b)
    {
      kfree(b);
      csi_dev->discard_buffer_dma = 0;
    }
#endif

#if TRACE_MX6S_STOP_STREAMING
    dev_err(csi_dev->dev, "%s: Exit\n", __func__);
#endif

}

static struct vb2_ops mx6s_videobuf_ops = {
    .queue_setup     = mx6s_videobuf_setup,
    .buf_prepare     = mx6s_videobuf_prepare,
    .buf_queue       = mx6s_videobuf_queue,
    .wait_prepare    = vb2_ops_wait_prepare,
    .wait_finish     = vb2_ops_wait_finish,
    .start_streaming = mx6s_start_streaming,
    .stop_streaming  = mx6s_stop_streaming,
};

/****** mx6s_csi_frame_done = Frame done = 12.2020 **********************/
static void mx6s_csi_frame_done(struct mx6s_csi_dev *csi_dev,
        int bufnum, bool err)
{

#define TRACE_MX6S_CSI_FRAME_DONE   0   /* DDD - mx6s_csi_frame_done - trace */

    struct mx6s_buf_internal *ibuf;
    struct mx6s_buffer *buf;
    struct vb2_buffer *vb;
    unsigned long phys;
    unsigned int phys_fb1;
    unsigned int phys_fb2;

    ibuf = list_first_entry(&csi_dev->active_bufs, struct mx6s_buf_internal,
                   queue);

    if (ibuf->discard) {
        /*
         * Discard buffer must not be returned to user space.
         * Just return it to the discard queue.
         */
        list_move_tail(csi_dev->active_bufs.next, &csi_dev->discard);
    } else {
        buf = mx6s_ibuf_to_buf(ibuf);

        vb = &buf->vb.vb2_buf;
        phys = vb2_dma_contig_plane_dma_addr(vb, 0);
        if (bufnum == 1) {
            phys_fb2 = csi_read(csi_dev, CSI_CSIDMASA_FB2);
            if (phys_fb2 != (u32)phys) {
                dev_err(csi_dev->dev, "%lx != %x\n", phys,
                    phys_fb2);
            }
        } else {
            phys_fb1 = csi_read(csi_dev, CSI_CSIDMASA_FB1);
            if (phys_fb1 != (u32)phys) {
                dev_err(csi_dev->dev, "%lx != %x\n", phys,
                    phys_fb1);
            }
        }
        dev_dbg(csi_dev->dev, "%s (vb=0x%p) 0x%p %lu\n", __func__, vb,
                vb2_plane_vaddr(vb, 0),
                vb2_get_plane_payload(vb, 0));

        list_del_init(&buf->internal.queue);
        vb->timestamp =ktime_get_ns();
        to_vb2_v4l2_buffer(vb)->sequence = csi_dev->frame_count;
        if (err)
            vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);
        else
            vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
    }

    csi_dev->frame_count++;
    csi_dev->nextfb = (bufnum == 0 ? 1 : 0);

#if TRACE_MX6S_CSI_FRAME_DONE
    dev_err(csi_dev->dev, "%s: bufnum=%d frame_count=%d\n", __func__, bufnum, csi_dev->frame_count);
#endif

    /* Config discard buffer to active_bufs */
    if (list_empty(&csi_dev->capture)) {
        if (list_empty(&csi_dev->discard)) {
            dev_warn(csi_dev->dev,
                    "%s: trying to access empty discard list\n",
                    __func__);
            return;
        }

        ibuf = list_first_entry(&csi_dev->discard,
                    struct mx6s_buf_internal, queue);
        ibuf->bufnum = bufnum;

        list_move_tail(csi_dev->discard.next, &csi_dev->active_bufs);

        mx6s_update_csi_buf(csi_dev,
                    csi_dev->discard_buffer_dma, bufnum);
        return;
    }

    buf = list_first_entry(&csi_dev->capture, struct mx6s_buffer,
                   internal.queue);

    buf->internal.bufnum = bufnum;

    list_move_tail(csi_dev->capture.next, &csi_dev->active_bufs);

    vb = &buf->vb.vb2_buf;
    vb->state = VB2_BUF_STATE_ACTIVE;

    phys = vb2_dma_contig_plane_dma_addr(vb, 0);
    mx6s_update_csi_buf(csi_dev, phys, bufnum);
}

/****** mx6s_csi_irq_handler = CSI IRQ handler = 02.2021 ****************/
static irqreturn_t mx6s_csi_irq_handler(int irq, void *data)
{

#define TRACE_MX6S_CSI_IRQ_HANDLER      1   /* DDD - mx6s_csi_irq_handler - trace */
#define DUMP_SKIP_FRAME_STAT            0   /* DDD - mx6s_csi_irq_handler - dump skip frame statistics */
#define DISABLE_ADDR_CH_ERR_ERROR       0   /* DDD - mx6s_csi_irq_handler - disable base address change error   */
#define MX6S_CSI_IRQ_HANDLER_UDELAY     0   /* DDD - mx6s_csi_irq_handler - add delay in microseconds (us), 0=off */

    struct mx6s_csi_dev *csi_dev =  data;
    unsigned long status;
    u32 cr3, cr18;

#if DUMP_SKIP_FRAME_STAT
    static int skip_frame_cnt = 0;
    int stat_cnt = 20;
#endif

    spin_lock(&csi_dev->slock);


    status = csi_read(csi_dev, CSI_CSISR);
    csi_write(csi_dev, status, CSI_CSISR);

#if MX6S_CSI_IRQ_HANDLER_UDELAY
    udelay(MX6S_CSI_IRQ_HANDLER_UDELAY);
#endif

#if TRACE_MX6S_CSI_IRQ_HANDLER
//    dev_warn(csi_dev->dev, ".");
#endif

    if (list_empty(&csi_dev->active_bufs)) {
        dev_warn(csi_dev->dev,
                "%s: called while active list is empty\n",
                __func__);

        spin_unlock(&csi_dev->slock);
        return IRQ_HANDLED;
    }

    if (status & BIT_RFF_OR_INT) {
        dev_warn(csi_dev->dev, "%s Rx fifo overflow\n", __func__);
        if (csi_dev->soc->rx_fifo_rst)
            csi_error_recovery(csi_dev);
    }

    if (status & BIT_HRESP_ERR_INT) {
        dev_warn(csi_dev->dev, "%s Hresponse error detected\n",
            __func__);
        csi_error_recovery(csi_dev);
    }

#if DISABLE_ADDR_CH_ERR_ERROR
    status &= ~BIT_ADDR_CH_ERR_INT;
#endif

    if (status & BIT_ADDR_CH_ERR_INT) {
        /* Disable csi  */
        cr18 = csi_read(csi_dev, CSI_CSICR18);
        cr18 &= ~BIT_CSI_ENABLE;
        csi_write(csi_dev, cr18, CSI_CSICR18);

        /* DMA reflash */
        cr3 = csi_read(csi_dev, CSI_CSICR3);
        cr3 |= BIT_DMA_REFLASH_RFF;
        csi_write(csi_dev, cr3, CSI_CSICR3);

        /* Ensable csi  */
        cr18 |= BIT_CSI_ENABLE;
        csi_write(csi_dev, cr18, CSI_CSICR18);

        csi_dev->skipframe = 1;
        pr_debug("base address switching Change Err.\n");
//        pr_err("base address switching Change Err.\n");

#if TRACE_MX6S_CSI_IRQ_HANDLER
//        dev_warn(csi_dev->dev, "%s: BASEADDR_CHANGE_ERROR: CSI_CSISR bit-28\n", __func__);
//        pr_err("%s: base address switching err: CSI_CSICR18=0x%x\n", __func__, cr18);
//        pr_err("a-c-e");
#endif

    }


    if ((status & BIT_DMA_TSF_DONE_FB1) &&
        (status & BIT_DMA_TSF_DONE_FB2)) {
        /* For both FB1 and FB2 interrupter bits set case,
         * CSI DMA is work in one of FB1 and FB2 buffer,
         * but software can not know the state.
         * Skip it to avoid base address updated
         * when csi work in field0 and field1 will write to
         * new base address.
         * PDM TKT230775 */
        pr_err("Skip two frames\n");
    } else if (status & BIT_DMA_TSF_DONE_FB1) {
        if (csi_dev->nextfb == 0) {
            if (csi_dev->skipframe > 0)
            {
                csi_dev->skipframe--;
#if TRACE_MX6S_CSI_IRQ_HANDLER
                pr_err("adr-change-err 0");
#endif
            }
            else
            {
                mx6s_csi_frame_done(csi_dev, 0, false);
#if TRACE_MX6S_CSI_IRQ_HANDLER
//                pr_err("OK 0");
#endif
            }
        } else {
#if TRACE_MX6S_CSI_IRQ_HANDLER
//            pr_warn("skip frame 0\n");
            pr_warn("skip 0\n");
#endif

#if DUMP_SKIP_FRAME_STAT
            skip_frame_cnt++;
            if(skip_frame_cnt >= stat_cnt)
            {
              pr_warn("%d skipped frames\n", skip_frame_cnt);
              skip_frame_cnt = 0;
            }
#endif
        }

    } else if (status & BIT_DMA_TSF_DONE_FB2) {
        if (csi_dev->nextfb == 1) {
            if (csi_dev->skipframe > 0)
            {
                csi_dev->skipframe--;
#if TRACE_MX6S_CSI_IRQ_HANDLER
                pr_err("adr-change-err 1");
#endif
            }
            else
            {
                mx6s_csi_frame_done(csi_dev, 1, false);
#if TRACE_MX6S_CSI_IRQ_HANDLER
//                pr_err("OK 1");
#endif
            }
        } else {
#if TRACE_MX6S_CSI_IRQ_HANDLER
//            pr_warn("skip frame 1\n");
            pr_warn("skip 1\n");
#endif
#if DUMP_SKIP_FRAME_STAT
            skip_frame_cnt++;
            if(skip_frame_cnt >= stat_cnt)
            {
              pr_warn("%d skipped frames\n", skip_frame_cnt);
              skip_frame_cnt = 0;
            }
#endif
        }
    }

    spin_unlock(&csi_dev->slock);

    return IRQ_HANDLED;
}

/*
 * File operations for the device
 */
static int mx6s_csi_open(struct file *file)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;
    struct vb2_queue *q = &csi_dev->vb2_vidq;
    int ret = 0;

    file->private_data = csi_dev;

    if (mutex_lock_interruptible(&csi_dev->lock))
        return -ERESTARTSYS;

    if (csi_dev->open_count++ == 0) {
        q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        q->io_modes = VB2_MMAP | VB2_USERPTR;
        q->drv_priv = csi_dev;
        q->ops = &mx6s_videobuf_ops;
        q->mem_ops = &vb2_dma_contig_memops;
        q->buf_struct_size = sizeof(struct mx6s_buffer);
        q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
        q->lock = &csi_dev->lock;

        ret = vb2_queue_init(q);
        if (ret < 0)
            goto unlock;

        pm_runtime_get_sync(csi_dev->dev);

        request_bus_freq(BUS_FREQ_HIGH);

        v4l2_subdev_call(sd, core, s_power, 1);
        mx6s_csi_init(csi_dev);

    }
    mutex_unlock(&csi_dev->lock);

    return ret;
unlock:
    mutex_unlock(&csi_dev->lock);
    return ret;
}

static int mx6s_csi_close(struct file *file)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    mutex_lock(&csi_dev->lock);

    if (--csi_dev->open_count == 0) {
        vb2_queue_release(&csi_dev->vb2_vidq);

        mx6s_csi_deinit(csi_dev);
        v4l2_subdev_call(sd, core, s_power, 0);

        file->private_data = NULL;

        release_bus_freq(BUS_FREQ_HIGH);

        pm_runtime_put_sync_suspend(csi_dev->dev);
    }
    mutex_unlock(&csi_dev->lock);

    return 0;
}

/****** mx6s_csi_read = CSI read = 01.2021 ******************************/
static ssize_t mx6s_csi_read(struct file *file, char __user *buf,
                   size_t count, loff_t *ppos)
{

#define TRACE_MX6S_CSI_READ     0   /* DDD - mx6s_csi_read - trace */

    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    int ret;

    dev_dbg(csi_dev->dev, "read called, buf %p\n", buf);
#if TRACE_MX6S_CSI_READ
//    pr_err("%s: count=%d, buf=%p\n", __func__, count, buf);
#endif

    mutex_lock(&csi_dev->lock);
    ret = vb2_read(&csi_dev->vb2_vidq, buf, count, ppos,
                file->f_flags & O_NONBLOCK);

#if TRACE_MX6S_CSI_READ
    dev_err(csi_dev->dev, "%s: count=%d ppos=%d, buf=%p\n", __func__, (int)count, (int)*ppos, buf);
#endif

    mutex_unlock(&csi_dev->lock);
    return ret;
}

/****** mx6s_csi_mmap = CSI MMAP read = 01.2021 *************************/
static int mx6s_csi_mmap(struct file *file, struct vm_area_struct *vma)
{

#define TRACE_MX6S_CSI_MMAP     0   /* DDD - mx6s_csi_mmap - trace */

    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    int ret;

    if (mutex_lock_interruptible(&csi_dev->lock))
        return -ERESTARTSYS;

    ret = vb2_mmap(&csi_dev->vb2_vidq, vma);
    mutex_unlock(&csi_dev->lock);

    pr_debug("vma start=0x%08lx, size=%ld, ret=%d\n",
        (unsigned long)vma->vm_start,
        (unsigned long)vma->vm_end-(unsigned long)vma->vm_start,
        ret);

#if TRACE_MX6S_CSI_MMAP
    dev_err(csi_dev->dev, "%s: vma start=0x%08lx, size=%ld, ret=%d\n", __func__,
        (unsigned long)vma->vm_start,
        (unsigned long)vma->vm_end-(unsigned long)vma->vm_start,
        ret);
#endif

    return ret;
}

static struct v4l2_file_operations mx6s_csi_fops = {
    .owner      = THIS_MODULE,
    .open       = mx6s_csi_open,
    .release    = mx6s_csi_close,
    .read       = mx6s_csi_read,
    .poll       = vb2_fop_poll,
    .unlocked_ioctl = video_ioctl2, /* V4L2 ioctl handler */
    .mmap       = mx6s_csi_mmap,
//    .mmap       = vb2_fop_mmap,
};

/*
 * Video node IOCTLs
 */
static int mx6s_vidioc_enum_input(struct file *file, void *priv,
                 struct v4l2_input *inp)
{
    if (inp->index != 0)
        return -EINVAL;

    /* default is camera */
    inp->type = V4L2_INPUT_TYPE_CAMERA;
    strcpy(inp->name, "Camera");

    return 0;
}

static int mx6s_vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
    *i = 0;

    return 0;
}

static int mx6s_vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
    if (i > 0)
        return -EINVAL;

    return 0;
}

static int mx6s_vidioc_querystd(struct file *file, void *priv, v4l2_std_id *a)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    return v4l2_subdev_call(sd, video, querystd, a);
}

static int mx6s_vidioc_s_std(struct file *file, void *priv, v4l2_std_id a)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    return v4l2_subdev_call(sd, video, s_std, a);
}

static int mx6s_vidioc_g_std(struct file *file, void *priv, v4l2_std_id *a)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    return v4l2_subdev_call(sd, video, g_std, a);
}

static int mx6s_vidioc_reqbufs(struct file *file, void *priv,
                  struct v4l2_requestbuffers *p)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);

    WARN_ON(priv != file->private_data);

    return vb2_reqbufs(&csi_dev->vb2_vidq, p);
}

static int mx6s_vidioc_querybuf(struct file *file, void *priv,
                   struct v4l2_buffer *p)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    int ret;

    WARN_ON(priv != file->private_data);

    ret = vb2_querybuf(&csi_dev->vb2_vidq, p);

    if (!ret) {
        /* return physical address */
        struct vb2_buffer *vb = csi_dev->vb2_vidq.bufs[p->index];
        if (p->flags & V4L2_BUF_FLAG_MAPPED)
            p->m.offset = vb2_dma_contig_plane_dma_addr(vb, 0);
    }
    return ret;
}

static int mx6s_vidioc_qbuf(struct file *file, void *priv,
               struct v4l2_buffer *p)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);

    WARN_ON(priv != file->private_data);

    return vb2_qbuf(&csi_dev->vb2_vidq, NULL, p);
}

static int mx6s_vidioc_dqbuf(struct file *file, void *priv,
                struct v4l2_buffer *p)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);

    WARN_ON(priv != file->private_data);

    return vb2_dqbuf(&csi_dev->vb2_vidq, p, file->f_flags & O_NONBLOCK);
}

/****** mx6s_vidioc_enum_fmt_vid_cap = Enum fmt = 11.2020 ***************/
static int mx6s_vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
                       struct v4l2_fmtdesc *f)
{

#define TRACE_MX6S_VIDIOC_ENUM_FMT_VID_CAP  0   /* DDD - mx6s_vidioc_enum_fmt_vid_cap - trace */

    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;
    struct v4l2_subdev_mbus_code_enum code = {
        .which = V4L2_SUBDEV_FORMAT_ACTIVE,
        .index = f->index,
    };
    struct mx6s_fmt *fmt;
    int ret;

    WARN_ON(priv != file->private_data);

#if TRACE_MX6S_VIDIOC_ENUM_FMT_VID_CAP
//    dev_err(csi_dev->dev, "%s: -------\n", __func__);
#endif

    ret = v4l2_subdev_call(sd, pad, enum_mbus_code, NULL, &code);
    if (ret < 0) {
        /* no more formats */
        dev_dbg(csi_dev->dev, "No more fmt\n");
        return -EINVAL;
    }

    fmt = format_by_mbus(code.code);
    if (!fmt) {
        dev_err(csi_dev->dev, "mbus (0x%08x) invalid.\n", code.code);
        return -EINVAL;
    }

    strlcpy(f->description, fmt->name, sizeof(f->description));
    f->pixelformat = fmt->pixelformat;


//struct mx6s_fmt {
//    char  name[32];
//    u32   fourcc;       /* v4l2 format id */
//    u32   pixelformat;
//    u32   mbus_code;
//    int   bpp;
//};

#if TRACE_MX6S_VIDIOC_ENUM_FMT_VID_CAP
{
    int pix_fmt = fmt->pixelformat;
    dev_err(csi_dev->dev, "%s: pix_fmt=0x%08x '%c%c%c%c'\n", __func__,
                        pix_fmt,
                        (char)((pix_fmt      ) & 0xFF),
                        (char)((pix_fmt >>  8) & 0xFF),
                        (char)((pix_fmt >> 16) & 0xFF),
                        (char)((pix_fmt >> 24) & 0xFF));
/*
    dev_err(csi_dev->dev, "%s: dx,dy=%d,%d bytesperline=%d sizeimage=%d pixfmt=0x%08x '%c%c%c%c'\n", __func__,
                        pix->width, pix->height,
                        pix->bytesperline, pix->sizeimage,
                        pix_fmt,
                        (char)((pix_fmt      ) & 0xFF),
                        (char)((pix_fmt >>  8) & 0xFF),
                        (char)((pix_fmt >> 16) & 0xFF),
                        (char)((pix_fmt >> 24) & 0xFF));
*/
}
#endif

    return 0;
}

/****** mx6s_vidioc_try_fmt_vid_cap = Try format = 11.2020 **************/
static int mx6s_vidioc_try_fmt_vid_cap(struct file *file, void *priv,
                      struct v4l2_format *f)
{

#define TRACE_MX6S_VIDIOC_TRY_FMT_VID_CAP   0   /* DDD - mx6s_vidioc_try_fmt_vid_cap - trace */

    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;
    struct v4l2_pix_format *pix = &f->fmt.pix;
    struct v4l2_subdev_format format = {
        .which = V4L2_SUBDEV_FORMAT_ACTIVE,
    };
    struct mx6s_fmt *fmt;
    int ret;

#if TRACE_MX6S_VIDIOC_TRY_FMT_VID_CAP
    dev_err(csi_dev->dev, "%s:... \n", __func__);
#endif

    fmt = format_by_fourcc(f->fmt.pix.pixelformat);
    if (!fmt) {
        dev_err(csi_dev->dev, "Fourcc format (0x%08x) invalid.",
            f->fmt.pix.pixelformat);
        return -EINVAL;
    }

    if (f->fmt.pix.width == 0 || f->fmt.pix.height == 0) {
        dev_err(csi_dev->dev, "width %d, height %d is too small.\n",
            f->fmt.pix.width, f->fmt.pix.height);
        return -EINVAL;
    }

    v4l2_fill_mbus_format(&format.format, pix, fmt->mbus_code);
//    dev_err(csi_dev->dev, "%s: Bef set_fmt\n", __func__);
    ret = v4l2_subdev_call(sd, pad, set_fmt, NULL, &format);
//    dev_err(csi_dev->dev, "%s: Aft set_fmt\n", __func__);
    v4l2_fill_pix_format(pix, &format.format);

    if (pix->field != V4L2_FIELD_INTERLACED)
        pix->field = V4L2_FIELD_NONE;

    pix->sizeimage = fmt->bpp * pix->height * pix->width;
    pix->bytesperline = fmt->bpp * pix->width;

#if TRACE_MX6S_VIDIOC_TRY_FMT_VID_CAP
{
    int pix_fmt = pix->pixelformat;
    dev_err(csi_dev->dev, "%s: dx,dy=%d,%d bytesperline=%d sizeimage=%d pixfmt=0x%08x '%c%c%c%c'\n", __func__,
                        pix->width, pix->height,
                        pix->bytesperline, pix->sizeimage,
                        pix_fmt,
                        (char)((pix_fmt      ) & 0xFF),
                        (char)((pix_fmt >>  8) & 0xFF),
                        (char)((pix_fmt >> 16) & 0xFF),
                        (char)((pix_fmt >> 24) & 0xFF));
}
#endif

#if VC_MIPI
// ???    memcpy(&mx6s_pix_fmt, pix, sizeof(struct v4l2_pix_format));
    memcpy(&vc_mipi_csi_dev->pix, pix, sizeof(struct v4l2_pix_format));
#endif
    return ret;
}

/****** mx6s_vidioc_s_fmt_vid_cap = Set format = 11.2020 ****************/
/*
 * The real work of figuring out a workable format.
 */
static int mx6s_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
                    struct v4l2_format *f)
{

#define TRACE_MX6S_VIDIOC_S_FMT_VID_CAP     0   /* DDD - mx6s_vidioc_s_fmt_vid_cap - trace */
//#define MX6S_VIDIOC_S_FMT_VID_CAP_SAVE_PIXFMT   1   /* CCC - mx6s_vidioc_s_fmt_vid_cap - save pixel format: */
//                                                    /*    1 : 8-bit gray format: v4l2-ctl saves  frame into file with zero size !? */

    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    int ret;

#if TRACE_MX6S_VIDIOC_S_FMT_VID_CAP
    dev_err(csi_dev->dev, "%s:... \n", __func__);
#endif

    ret = mx6s_vidioc_try_fmt_vid_cap(file, csi_dev, f);
    if (ret < 0)
        return ret;

    csi_dev->fmt           = format_by_fourcc(f->fmt.pix.pixelformat);
    csi_dev->mbus_code     = csi_dev->fmt->mbus_code;
    csi_dev->pix.width     = f->fmt.pix.width;
    csi_dev->pix.height    = f->fmt.pix.height;
    csi_dev->pix.sizeimage = f->fmt.pix.sizeimage;
    csi_dev->pix.field     = f->fmt.pix.field;
    csi_dev->type          = f->type;

    csi_dev->pix.bytesperline = f->fmt.pix.bytesperline;

//#if MX6S_VIDIOC_S_FMT_VID_CAP_SAVE_PIXFMT  // 8-bit gray format: v4l2-ctl saves frame(s) into file with zero size !?
#if VC_MIPI
    csi_dev->pix.pixelformat  = f->fmt.pix.pixelformat;
#endif

    dev_dbg(csi_dev->dev, "set to pixelformat '%4.6s'\n",
            (char *)&csi_dev->fmt->name);

#if TRACE_MX6S_VIDIOC_S_FMT_VID_CAP
{
    int pix_fmt = csi_dev->pix.pixelformat;
    dev_err(csi_dev->dev, "%s: sizeimage=%d pixelformat_name='%4.6s' pixfmt=0x%08x '%c%c%c%c'\n", __func__,
                        csi_dev->pix.sizeimage, (char *)&csi_dev->fmt->name,
                        pix_fmt,
                        (char)((pix_fmt      ) & 0xFF),
                        (char)((pix_fmt >>  8) & 0xFF),
                        (char)((pix_fmt >> 16) & 0xFF),
                        (char)((pix_fmt >> 24) & 0xFF));
//    dev_err(csi_dev->dev, "%s: field=%d type=%d\n", __func__, csi_dev->pix.field, csi_dev->type);
}
#endif

    /* Config csi */
    mx6s_configure_csi(csi_dev);

    return 0;
}

/****** mx6s_vidioc_g_fmt_vid_cap = Get format = 11.2020 ****************/
static int mx6s_vidioc_g_fmt_vid_cap(struct file *file, void *priv,
                    struct v4l2_format *f)
{

#define TRACE_MX6S_VIDIOC_G_FMT_VID_CAP     0   /* DDD - mx6s_vidioc_g_fmt_vid_cap - trace */

    struct mx6s_csi_dev *csi_dev = video_drvdata(file);

#if TRACE_MX6S_VIDIOC_G_FMT_VID_CAP
    dev_err(csi_dev->dev, "%s:... \n", __func__);
#endif

    WARN_ON(priv != file->private_data);

    f->fmt.pix = csi_dev->pix;

    return 0;
}

static int mx6s_vidioc_querycap(struct file *file, void  *priv,
                   struct v4l2_capability *cap)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);

    WARN_ON(priv != file->private_data);

    /* cap->name is set by the friendly caller:-> */
    strlcpy(cap->driver, MX6S_CAM_DRV_NAME, sizeof(cap->driver));
    strlcpy(cap->card, MX6S_CAM_DRIVER_DESCRIPTION, sizeof(cap->card));
    snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
         dev_name(csi_dev->dev));

#if VC_MIPI
//    cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_READWRITE;
    cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
    cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
#else
    cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
    cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
#endif
    return 0;
}

static int mx6s_vidioc_expbuf(struct file *file, void *priv,
                 struct v4l2_exportbuffer *eb)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);

    if (eb->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
        return vb2_expbuf(&csi_dev->vb2_vidq, eb);

    return -EINVAL;
}

static int mx6s_vidioc_streamon(struct file *file, void *priv,
                   enum v4l2_buf_type i)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;
    int ret;

    WARN_ON(priv != file->private_data);

    if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE)
        return -EINVAL;

    ret = vb2_streamon(&csi_dev->vb2_vidq, i);
    if (!ret)
        v4l2_subdev_call(sd, video, s_stream, 1);

    return ret;
}

static int mx6s_vidioc_streamoff(struct file *file, void *priv,
                enum v4l2_buf_type i)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    WARN_ON(priv != file->private_data);

    if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE)
        return -EINVAL;

    /*
     * This calls buf_release from host driver's videobuf_queue_ops for all
     * remaining buffers. When the last buffer is freed, stop capture
     */
    vb2_streamoff(&csi_dev->vb2_vidq, i);

    v4l2_subdev_call(sd, video, s_stream, 0);

    return 0;
}

static int mx6s_vidioc_g_pixelaspect(struct file *file, void *fh,
                   int type, struct v4l2_fract *f)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);

    if (type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
        return -EINVAL;
    dev_dbg(csi_dev->dev, "G_PIXELASPECT not implemented\n");

    return 0;
}

static int mx6s_vidioc_g_selection(struct file *file, void *priv,
                 struct v4l2_selection *s)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);

    if (s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
        return -EINVAL;
    dev_dbg(csi_dev->dev, "VIDIOC_G_SELECTION not implemented\n");

    return 0;
}

static int mx6s_vidioc_s_selection(struct file *file, void *priv,
                 struct v4l2_selection *s)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);

    if (s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
        return -EINVAL;

    dev_dbg(csi_dev->dev, "VIDIOC_S_SELECTION not implemented\n");

    return 0;
}

static int mx6s_vidioc_g_parm(struct file *file, void *priv,
                 struct v4l2_streamparm *a)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    return v4l2_subdev_call(sd, video, g_parm, a);
}

static int mx6s_vidioc_s_parm(struct file *file, void *priv,
                struct v4l2_streamparm *a)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    return v4l2_subdev_call(sd, video, s_parm, a);
}

static int mx6s_vidioc_enum_framesizes(struct file *file, void *priv,
                     struct v4l2_frmsizeenum *fsize)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;
    struct mx6s_fmt *fmt;
    struct v4l2_subdev_frame_size_enum fse = {
        .index = fsize->index,
        .which = V4L2_SUBDEV_FORMAT_ACTIVE,
    };
    int ret;

    fmt = format_by_fourcc(fsize->pixel_format);

    if(fmt == NULL)
        return -EINVAL;

    if (fmt->pixelformat != fsize->pixel_format)
        return -EINVAL;
    fse.code = fmt->mbus_code;

    ret = v4l2_subdev_call(sd, pad, enum_frame_size, NULL, &fse);
    if (ret)
        return ret;

    if (fse.min_width == fse.max_width &&
        fse.min_height == fse.max_height) {
        fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
        fsize->discrete.width = fse.min_width;
        fsize->discrete.height = fse.min_height;
        return 0;
    }

    fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
    fsize->stepwise.min_width = fse.min_width;
    fsize->stepwise.max_width = fse.max_width;
    fsize->stepwise.min_height = fse.min_height;
    fsize->stepwise.max_height = fse.max_height;
    fsize->stepwise.step_width = 1;
    fsize->stepwise.step_height = 1;

    return 0;
}

static int mx6s_vidioc_enum_frameintervals(struct file *file, void *priv,
        struct v4l2_frmivalenum *interval)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;
    struct mx6s_fmt *fmt;
    struct v4l2_subdev_frame_interval_enum fie = {
        .index = interval->index,
        .width = interval->width,
        .height = interval->height,
        .which = V4L2_SUBDEV_FORMAT_ACTIVE,
    };
    int ret;

    fmt = format_by_fourcc(interval->pixel_format);

    if(fmt == NULL)
        return -EINVAL;

    if (fmt->pixelformat != interval->pixel_format)
        return -EINVAL;
    fie.code = fmt->mbus_code;

    ret = v4l2_subdev_call(sd, pad, enum_frame_interval, NULL, &fie);
    if (ret)
        return ret;
    interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
    interval->discrete = fie.interval;
    return 0;
}

#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD1  /* [[[ */

static int mx6s_vidioc_queryctrl(struct file *file, void *fh,
                   struct v4l2_queryctrl *a)
{

#define TRACE_MX6S_VIDIOC_QUERYCTRL         0   /* DDD - mx6s_vidioc_queryctrl - trace */

    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

#if TRACE_MX6S_VIDIOC_QUERYCTRL
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    dev_err(csi_dev->dev, "[vc_mipi] %s: \n", __func__);
}
#endif

    return v4l2_subdev_call(sd, core, queryctrl, a);
}

static int mx6s_vidioc_query_ext_ctrl(struct file *file, void *fh,
                   struct v4l2_query_ext_ctrl *qec)
{

#define TRACE_MX6S_VIDIOC_QUERY_EXT_CTRL    0   /* DDD - mx6s_vidioc_query_ext_ctrl - trace */
//#define CALL_VC_MIPI_DRIVER_FUNC            1   /* CCC - mx6s_vidioc_query_ext_ctrl - call VC MIPI driver function */

#if TRACE_MX6S_VIDIOC_QUERY_EXT_CTRL
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    dev_err(csi_dev->dev, "[vc_mipi] %s: \n", __func__);
}
#endif

// #if CALL_VC_MIPI_DRIVER_FUNC
#if VC_MIPI
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    return v4l2_subdev_call(sd, core, queryextctrl, qec);
}
#else   // [[[
{
    struct v4l2_queryctrl qc = {
        .id = qec->id
    };
    int ret;
    ret = mx6s_vidioc_queryctrl(file, fh, &qc);

    if (ret)
        return ret;

    qec->id = qc.id;
    qec->type = qc.type;
    strlcpy(qec->name, qc.name, sizeof(qec->name));
    qec->maximum = qc.maximum;
    qec->minimum = qc.minimum;
    qec->step = qc.step;
    qec->default_value = qc.default_value;
    qec->flags = qc.flags;
    qec->elem_size = 4;
    qec->elems = 1;
    qec->nr_of_dims = 0;
    memset(qec->dims, 0, sizeof(qec->dims));
    memset(qec->reserved, 0, sizeof(qec->reserved));

    return 0;
}
#endif  // ]]]

}

static int mx6s_vidioc_querymenu(struct file *file, void *fh,
                   struct v4l2_querymenu *qm)
{

#define TRACE_MX6S_VIDIOC_QUERYMENU         0   /* DDD - mx6s_vidioc_querymenu - trace */

    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;


#if TRACE_MX6S_VIDIOC_QUERYMENU
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    dev_err(csi_dev->dev, "%s: \n", __func__);
}
#endif

    return v4l2_subdev_call(sd, core, querymenu, qm);
}

static int mx6s_vidioc_g_ctrl(struct file *file, void *fh,
                   struct v4l2_control *a)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    return v4l2_subdev_call(sd, core, g_ctrl, a);
}

static int mx6s_vidioc_s_ctrl(struct file *file, void *fh,
                   struct v4l2_control *a)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    return v4l2_subdev_call(sd, core, s_ctrl, a);
}

static int mx6s_vidioc_g_ext_ctrls(struct file *file, void *fh,
                 struct v4l2_ext_controls *a)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    return v4l2_subdev_call(sd, core, g_ext_ctrls, a);
}

static int mx6s_vidioc_s_ext_ctrls(struct file *file, void *fh,
                 struct v4l2_ext_controls *a)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    return v4l2_subdev_call(sd, core, s_ext_ctrls, a);
}

static int mx6s_vidioc_try_ext_ctrls(struct file *file, void *fh,
                 struct v4l2_ext_controls *a)
{
    struct mx6s_csi_dev *csi_dev = video_drvdata(file);
    struct v4l2_subdev *sd = csi_dev->sd;

    return v4l2_subdev_call(sd, core, try_ext_ctrls, a);

}

#endif  /* ]]] */

static const struct v4l2_ioctl_ops mx6s_csi_ioctl_ops = {
    .vidioc_querycap          = mx6s_vidioc_querycap,
    .vidioc_enum_fmt_vid_cap  = mx6s_vidioc_enum_fmt_vid_cap,
    .vidioc_try_fmt_vid_cap   = mx6s_vidioc_try_fmt_vid_cap,
    .vidioc_g_fmt_vid_cap     = mx6s_vidioc_g_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap     = mx6s_vidioc_s_fmt_vid_cap,
    .vidioc_g_pixelaspect     = mx6s_vidioc_g_pixelaspect,
    .vidioc_s_selection   = mx6s_vidioc_s_selection,
    .vidioc_g_selection   = mx6s_vidioc_g_selection,
    .vidioc_reqbufs       = mx6s_vidioc_reqbufs,
    .vidioc_querybuf      = mx6s_vidioc_querybuf,
    .vidioc_qbuf          = mx6s_vidioc_qbuf,
    .vidioc_dqbuf         = mx6s_vidioc_dqbuf,
    .vidioc_g_std         = mx6s_vidioc_g_std,
    .vidioc_s_std         = mx6s_vidioc_s_std,
    .vidioc_querystd      = mx6s_vidioc_querystd,
    .vidioc_enum_input    = mx6s_vidioc_enum_input,
    .vidioc_g_input       = mx6s_vidioc_g_input,
    .vidioc_s_input       = mx6s_vidioc_s_input,
    .vidioc_expbuf        = mx6s_vidioc_expbuf,
    .vidioc_streamon      = mx6s_vidioc_streamon,
    .vidioc_streamoff     = mx6s_vidioc_streamoff,
    .vidioc_g_parm        = mx6s_vidioc_g_parm,
    .vidioc_s_parm        = mx6s_vidioc_s_parm,
    .vidioc_enum_framesizes = mx6s_vidioc_enum_framesizes,
    .vidioc_enum_frameintervals = mx6s_vidioc_enum_frameintervals,

#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD1  /* [[[ */
    .vidioc_queryctrl      = mx6s_vidioc_queryctrl,
    .vidioc_query_ext_ctrl = mx6s_vidioc_query_ext_ctrl,
    .vidioc_querymenu      = mx6s_vidioc_querymenu,
    .vidioc_g_ctrl         = mx6s_vidioc_g_ctrl,
    .vidioc_s_ctrl         = mx6s_vidioc_s_ctrl,
    .vidioc_g_ext_ctrls    = mx6s_vidioc_g_ext_ctrls,
    .vidioc_s_ext_ctrls    = mx6s_vidioc_s_ext_ctrls,
    .vidioc_try_ext_ctrls  = mx6s_vidioc_try_ext_ctrls,
#endif  /* ]]] */
};

static int subdev_notifier_bound(struct v4l2_async_notifier *notifier,
                struct v4l2_subdev *subdev,
                struct v4l2_async_subdev *asd)
{
    struct mx6s_csi_dev *csi_dev = notifier_to_mx6s_dev(notifier);

    /* Find platform data for this sensor subdev */
    if (csi_dev->asd.match.fwnode == dev_fwnode(subdev->dev))
        csi_dev->sd = subdev;

    if (subdev == NULL)
        return -EINVAL;

    v4l2_info(&csi_dev->v4l2_dev, "Registered sensor subdevice: %s\n",
          subdev->name);

    return 0;
}

static int mx6s_csi_mode_sel(struct mx6s_csi_dev *csi_dev)
{
    struct device_node *np = csi_dev->dev->of_node;
    struct device_node *node;
    phandle phandle;
    u32 out_val[3];
    int ret = 0;

    if (of_get_property(np, "fsl,mipi-mode", NULL))
        csi_dev->csi_mipi_mode = true;
    else {
        csi_dev->csi_mipi_mode = false;
        return ret;
    }

    ret = of_property_read_u32_array(np, "csi-mux-mipi", out_val, 3);
    if (ret) {
        dev_dbg(csi_dev->dev, "no csi-mux-mipi property found\n");
    } else {
        phandle = *out_val;

        node = of_find_node_by_phandle(phandle);
        if (!node) {
            dev_dbg(csi_dev->dev, "not find gpr node by phandle\n");
            ret = PTR_ERR(node);
        }
        csi_dev->csi_mux.gpr = syscon_node_to_regmap(node);
        if (IS_ERR(csi_dev->csi_mux.gpr)) {
            dev_err(csi_dev->dev, "failed to get gpr regmap\n");
            ret = PTR_ERR(csi_dev->csi_mux.gpr);
        }
        of_node_put(node);
        if (ret < 0)
            return ret;

        csi_dev->csi_mux.req_gpr = out_val[1];
        csi_dev->csi_mux.req_bit = out_val[2];

        regmap_update_bits(csi_dev->csi_mux.gpr, csi_dev->csi_mux.req_gpr,
            1 << csi_dev->csi_mux.req_bit, 1 << csi_dev->csi_mux.req_bit);
    }
    return ret;
}

static const struct v4l2_async_notifier_operations mx6s_capture_async_ops = {
    .bound = subdev_notifier_bound,
};

static int mx6s_csi_two_8bit_sensor_mode_sel(struct mx6s_csi_dev *csi_dev)
{

#define TRACE_MX6S_CSI_TWO_8BIT_SENSOR_MODE_SEL     0   /* DDD - mx6s_csi_two_8bit_sensor_mode_sel - trace */

    struct device_node *np = csi_dev->dev->of_node;

    if (of_get_property(np, "fsl,two-8bit-sensor-mode", NULL))
        csi_dev->csi_two_8bit_sensor_mode = true;
    else {
        csi_dev->csi_two_8bit_sensor_mode = false;
    }

#if TRACE_MX6S_CSI_TWO_8BIT_SENSOR_MODE_SEL
    dev_err(csi_dev->dev, "%s: csi_two_8bit_sensor_mode = %d\n", __func__, (int)csi_dev->csi_two_8bit_sensor_mode);
#endif


// PK: Needs to be set dynamically in respect to 8-bit or 10/12-bit pixel format
//#if VC_MIPI
//// Force two_8bit_sensor_mode on 10/12-bit raw format */
////    switch (csi_dev->fmt->pixelformat)
////    {
////      case V4L2_PIX_FMT_Y10:
////      case V4L2_PIX_FMT_SRGGB10:
////      case V4L2_PIX_FMT_Y12:
////      case V4L2_PIX_FMT_SRGGB12:
//        csi_dev->csi_two_8bit_sensor_mode = true;
//#if TRACE_MX6S_CSI_TWO_8BIT_SENSOR_MODE_SEL
//        dev_err(csi_dev->dev, "%s: csi_two_8bit_sensor_mode: = true\n", __func__);
//#endif
////        break;
////    }
//#endif

    return 0;
}

static int mx6sx_register_subdevs(struct mx6s_csi_dev *csi_dev)
{
    struct device_node *parent = csi_dev->dev->of_node;
    struct device_node *node, *port, *rem;
    int ret;

    v4l2_async_notifier_init(&csi_dev->subdev_notifier);

    /* Attach sensors linked to csi receivers */
    for_each_available_child_of_node(parent, node) {
        if (of_node_cmp(node->name, "port"))
            continue;

        /* The csi node can have only port subnode. */
        port = of_get_next_child(node, NULL);
        if (!port)
            continue;
        rem = of_graph_get_remote_port_parent(port);
        of_node_put(port);
        if (rem == NULL) {
            v4l2_info(&csi_dev->v4l2_dev,
                        "Remote device at %s not found\n",
                        port->full_name);
            return -1;
        }

        csi_dev->asd.match_type = V4L2_ASYNC_MATCH_FWNODE;
        csi_dev->asd.match.fwnode = of_fwnode_handle(rem);

        ret = v4l2_async_notifier_add_subdev(&csi_dev->subdev_notifier,
                        &csi_dev->asd);
        if (ret) {
            of_node_put(rem);
        }

        of_node_put(rem);
        break;
    }

    csi_dev->subdev_notifier.ops = &mx6s_capture_async_ops;

    ret = v4l2_async_notifier_register(&csi_dev->v4l2_dev,
                    &csi_dev->subdev_notifier);
    if (ret)
        dev_err(csi_dev->dev,
                    "Error register async notifier regoster\n");

    return ret;
}

static int mx6s_csi_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    const struct of_device_id *of_id;
    struct mx6s_csi_dev *csi_dev;
    struct video_device *vdev;
    struct resource *res;
    int ret = 0;

    dev_err(dev, "[vc_mipi] %s: Probing MX6S CSI driver: %s\n", __func__, MX6S_CAM_DRV_NAME);
    dev_info(dev, "initialising\n");

    /* Prepare our private structure */
    csi_dev = devm_kzalloc(dev, sizeof(struct mx6s_csi_dev), GFP_ATOMIC);
    if (!csi_dev) {
        dev_err(dev, "Can't allocate private structure\n");
        return -ENODEV;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    csi_dev->irq = platform_get_irq(pdev, 0);
    if (res == NULL || csi_dev->irq < 0) {
        dev_err(dev, "Missing platform resources data\n");
        return -ENODEV;
    }

    csi_dev->regbase = devm_ioremap_resource(dev, res);
    if (IS_ERR(csi_dev->regbase)) {
        dev_err(dev, "Failed platform resources map\n");
        return -ENODEV;
    }

    /* init video dma queues */
    INIT_LIST_HEAD(&csi_dev->capture);
    INIT_LIST_HEAD(&csi_dev->active_bufs);
    INIT_LIST_HEAD(&csi_dev->discard);

    csi_dev->clk_disp_axi = devm_clk_get(dev, "disp-axi");
    if (IS_ERR(csi_dev->clk_disp_axi)) {
        dev_err(dev, "Could not get csi axi clock\n");
        return -ENODEV;
    }

    csi_dev->clk_disp_dcic = devm_clk_get(dev, "disp_dcic");
    if (IS_ERR(csi_dev->clk_disp_dcic)) {
        dev_err(dev, "Could not get disp dcic clock\n");
        return -ENODEV;
    }

    csi_dev->clk_csi_mclk = devm_clk_get(dev, "csi_mclk");
    if (IS_ERR(csi_dev->clk_csi_mclk)) {
        dev_err(dev, "Could not get csi mclk clock\n");
        return -ENODEV;
    }

    csi_dev->dev = dev;

    mx6s_csi_mode_sel(csi_dev);
    mx6s_csi_two_8bit_sensor_mode_sel(csi_dev);

    of_id = of_match_node(mx6s_csi_dt_ids, csi_dev->dev->of_node);
    if (!of_id)
        return -EINVAL;
    csi_dev->soc = of_id->data;

    snprintf(csi_dev->v4l2_dev.name,
         sizeof(csi_dev->v4l2_dev.name), "CSI");

#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD2
    ret = mx6s_init_controls(csi_dev);
    if (ret) {
        dev_err(dev, "Media controls register failed\n");
        return ret;
    }
    dev_err(dev, "[vc_mipi] %s: mx6s_init_controls() OK\n", __func__);
#endif

    ret = v4l2_device_register(dev, &csi_dev->v4l2_dev);
    if (ret < 0) {
        dev_err(dev, "v4l2_device_register() failed: %d\n", ret);
        ret = -ENODEV;
        goto err_v4l2;
    }

    /* initialize locks */
    mutex_init(&csi_dev->lock);
    spin_lock_init(&csi_dev->slock);

    /* Allocate memory for video device */
    vdev = video_device_alloc();
    if (vdev == NULL) {
        ret = -ENOMEM;
        goto err_vdev;
    }

    snprintf(vdev->name, sizeof(vdev->name), "mx6s-csi");

    vdev->v4l2_dev      = &csi_dev->v4l2_dev;
    vdev->fops          = &mx6s_csi_fops;
    vdev->ioctl_ops     = &mx6s_csi_ioctl_ops;
    vdev->release       = video_device_release;
    vdev->lock          = &csi_dev->lock;
    vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

    vdev->queue = &csi_dev->vb2_vidq;

    csi_dev->vdev = vdev;

    video_set_drvdata(csi_dev->vdev, csi_dev);
    mutex_lock(&csi_dev->lock);

    ret = video_register_device(csi_dev->vdev, VFL_TYPE_GRABBER, -1);
    if (ret < 0) {
        video_device_release(csi_dev->vdev);
        mutex_unlock(&csi_dev->lock);
        goto err_vdev;
    }

    /* install interrupt handler */
    if (devm_request_irq(dev, csi_dev->irq, mx6s_csi_irq_handler,
                0, "csi", (void *)csi_dev)) {
        mutex_unlock(&csi_dev->lock);
        dev_err(dev, "Request CSI IRQ failed.\n");
        ret = -ENODEV;
        goto err_irq;
    }

    mutex_unlock(&csi_dev->lock);

    ret = mx6sx_register_subdevs(csi_dev);
    if (ret < 0)
        goto err_irq;

#if VC_MIPI
    vc_mipi_csi_dev = csi_dev;
#endif

    pm_runtime_enable(csi_dev->dev);
    return 0;

err_irq:
    video_unregister_device(csi_dev->vdev);
err_vdev:
    v4l2_device_unregister(&csi_dev->v4l2_dev);
err_v4l2:
#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD2
    mx6s_free_controls(csi_dev);
#endif

    return ret;
}

static int mx6s_csi_remove(struct platform_device *pdev)
{
    struct v4l2_device *v4l2_dev = dev_get_drvdata(&pdev->dev);
    struct mx6s_csi_dev *csi_dev =
                container_of(v4l2_dev, struct mx6s_csi_dev, v4l2_dev);

    v4l2_async_notifier_unregister(&csi_dev->subdev_notifier);

    video_unregister_device(csi_dev->vdev);
    v4l2_device_unregister(&csi_dev->v4l2_dev);
#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD2
    mx6s_free_controls(csi_dev);
#endif

    pm_runtime_disable(csi_dev->dev);
    return 0;
}

static int mx6s_csi_runtime_suspend(struct device *dev)
{
    dev_dbg(dev, "csi v4l2 busfreq high release.\n");
    return 0;
}

static int mx6s_csi_runtime_resume(struct device *dev)
{
    dev_dbg(dev, "csi v4l2 busfreq high request.\n");
    return 0;
}

static const struct dev_pm_ops mx6s_csi_pm_ops = {
    SET_RUNTIME_PM_OPS(mx6s_csi_runtime_suspend, mx6s_csi_runtime_resume, NULL)
};

static const struct mx6s_csi_soc mx6s_soc = {
    .rx_fifo_rst = true,
    .baseaddr_switch = 0,
};
static const struct mx6s_csi_soc mx6sl_soc = {
    .rx_fifo_rst = false,
    .baseaddr_switch = 0,
};
static const struct mx6s_csi_soc mx8mq_soc = {
    .rx_fifo_rst = true,
    .baseaddr_switch = 0x80030,
//    .baseaddr_switch = 0x00030,
};

static const struct of_device_id mx6s_csi_dt_ids[] = {
    { .compatible = "fsl,imx6s-csi",
      .data = &mx6s_soc,
    },
    { .compatible = "fsl,imx6sl-csi",
      .data = &mx6sl_soc,
    },
    { .compatible = "fsl,imx8mq-csi",
      .data = &mx8mq_soc,
    },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mx6s_csi_dt_ids);

static struct platform_driver mx6s_csi_driver = {
    .driver     = {
        .name   = MX6S_CAM_DRV_NAME,
        .of_match_table = of_match_ptr(mx6s_csi_dt_ids),
        .pm = &mx6s_csi_pm_ops,
    },
    .probe  = mx6s_csi_probe,
    .remove = mx6s_csi_remove,
};

module_platform_driver(mx6s_csi_driver);

MODULE_DESCRIPTION("i.MX6Sx SoC Camera Host driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
MODULE_VERSION(MX6S_CAM_VERSION);
