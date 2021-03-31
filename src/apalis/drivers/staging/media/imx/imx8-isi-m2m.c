// SPDX-License-Identifier: GPL-2.0
/*
 * ISI V4L2 memory to memory driver for i.MX8QXP/QM platform
 *
 * ISI is a Image Sensor Interface of i.MX8QXP/QM platform, which
 * used to process image from camera sensor or memory to memory or DC
 *
 * Copyright (c) 2019 NXP Semiconductor
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/bug.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/pm_runtime.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/of_graph.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>

#include "imx8-isi-hw.h"
#include "imx8-common.h"

#define TRACE printk("        TRACE [vc-mipi] imx8-isi-m2m.c --->  %s : %d", __FUNCTION__, __LINE__);

#define to_isi_buffer(x)	\
	container_of((x), struct mxc_isi_buffer, v4l2_buf)

#define file_to_ctx(file)		\
	container_of(file->private_data, struct mxc_isi_ctx, fh);

#if defined(CONFIG_IMX8_ISI_CAPTURE)
extern struct mxc_isi_fmt mxc_isi_out_formats[9];
#else
static struct mxc_isi_fmt mxc_isi_out_formats[9] = {};
#endif

struct mxc_isi_fmt mxc_isi_input_formats[] = {
	/* Pixel link input format */
	{
		.name		= "XBGR32",
		.fourcc		= V4L2_PIX_FMT_XBGR32,
		.depth		= { 32 },
		.color =	MXC_ISI_M2M_IN_FMT_XRGB8,
		.memplanes	= 1,
		.colplanes	= 1,
	}, {
		.name		= "RGB565",
		.fourcc		= V4L2_PIX_FMT_RGB565,
		.depth		= { 16 },
		.color =	MXC_ISI_M2M_IN_FMT_RGB565,
		.memplanes	= 1,
		.colplanes	= 1,
	}, {
		.name		= "YUV24 (X-Y-U-V)",
		.fourcc		= V4L2_PIX_FMT_YUV24,
		.depth		= { 24 },
		.color = MXC_ISI_M2M_IN_FMT_YUV444_1P8P,
		.memplanes	= 1,
		.colplanes	= 1,
	}, {
		.name		= "YUV16 (X-Y-U-V)",
		.fourcc		= V4L2_PIX_FMT_YUYV,
		.depth		= { 16 },
		.color = MXC_ISI_M2M_IN_FMT_YUV422_1P8P,
		.memplanes	= 1,
		.colplanes	= 1,
	}, {
		.name		= "RGBA (R-G-B-A)",
		.fourcc		= V4L2_PIX_FMT_RGBA,
		.depth		= { 32 },
		.color = MXC_ISI_M2M_IN_FMT_XBGR8,
		.memplanes	= 1,
		.colplanes	= 1,
	}
};

static struct v4l2_m2m_buffer *to_v4l2_m2m_buffer(struct vb2_v4l2_buffer *vbuf)
{
	TRACE
	
	struct v4l2_m2m_buffer *b;

	b = container_of(vbuf, struct v4l2_m2m_buffer, vb);
	return b;
}

void mxc_isi_m2m_frame_write_done(struct mxc_isi_dev *mxc_isi)
{
	TRACE

	struct mxc_isi_m2m_dev *isi_m2m = mxc_isi->isi_m2m;
	struct v4l2_fh *fh;
	struct mxc_isi_ctx *curr_mxc_ctx;
	struct vb2_v4l2_buffer *src_vbuf, *dst_vbuf;
	struct mxc_isi_buffer *src_buf, *dst_buf;
	struct v4l2_m2m_buffer *b;

	dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s\n", __func__);

	curr_mxc_ctx = v4l2_m2m_get_curr_priv(isi_m2m->m2m_dev);
	if (!curr_mxc_ctx) {
		dev_err(&isi_m2m->pdev->dev,
			"[vc-mipi isi-m2m] Instance released before the end of transaction\n");
		return;
	}
	fh = &curr_mxc_ctx->fh;

	if (isi_m2m->aborting) {
		mxc_isi_channel_disable(mxc_isi);
		dev_warn(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] Aborting current job\n");
		goto job_finish;
	}

	src_vbuf = v4l2_m2m_next_src_buf(fh->m2m_ctx);
	if (!src_vbuf) {
		dev_err(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] No enought source buffers\n");
		goto job_finish;
	}
	src_buf = to_isi_buffer(src_vbuf);
	v4l2_m2m_src_buf_remove(fh->m2m_ctx);
	v4l2_m2m_buf_done(src_vbuf, VB2_BUF_STATE_DONE);

	if (!list_empty(&isi_m2m->out_active)) {
		dst_buf = list_first_entry(&isi_m2m->out_active,
					   struct mxc_isi_buffer, list);
		dst_vbuf = &dst_buf->v4l2_buf;
		list_del_init(&dst_buf->list);
		dst_buf->v4l2_buf.vb2_buf.timestamp = ktime_get_ns();
		v4l2_m2m_buf_done(dst_vbuf, VB2_BUF_STATE_DONE);

	}
	isi_m2m->frame_count++;

	dst_vbuf = v4l2_m2m_next_dst_buf(fh->m2m_ctx);
	if (dst_vbuf) {
		dst_vbuf->vb2_buf.state = VB2_BUF_STATE_ACTIVE;
		dst_buf = to_isi_buffer(dst_vbuf);
		dst_buf->v4l2_buf.sequence = isi_m2m->frame_count;
		mxc_isi_channel_set_outbuf(mxc_isi, dst_buf);
		v4l2_m2m_dst_buf_remove(fh->m2m_ctx);
		b = to_v4l2_m2m_buffer(dst_vbuf);
		list_add_tail(&b->list, &isi_m2m->out_active);
	}

job_finish:
	v4l2_m2m_job_finish(isi_m2m->m2m_dev, fh->m2m_ctx);
}

static void mxc_isi_m2m_device_run(void *priv)
{
	TRACE

	struct mxc_isi_ctx *mxc_ctx = priv;
	struct mxc_isi_m2m_dev *isi_m2m = mxc_ctx->isi_m2m;
	struct mxc_isi_dev *mxc_isi = mxc_isi_get_hostdata(isi_m2m->pdev);
	struct v4l2_fh *fh = &mxc_ctx->fh;
	struct vb2_v4l2_buffer *vbuf;
	struct mxc_isi_buffer *src_buf;
	unsigned long flags;

	dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s enter\n", __func__);

	spin_lock_irqsave(&isi_m2m->slock, flags);

	/* SRC */
	vbuf = v4l2_m2m_next_src_buf(fh->m2m_ctx);
	if (!vbuf) {
		dev_err(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] Null src buf\n");
		goto unlock;
	}

	src_buf = to_isi_buffer(vbuf);
	mxc_isi_channel_set_m2m_src_addr(mxc_isi, src_buf);
	mxc_isi_channel_enable(mxc_isi, mxc_isi->m2m_enabled);

unlock:
	spin_unlock_irqrestore(&isi_m2m->slock, flags);
}

static int mxc_isi_m2m_job_ready(void *priv)
{
	TRACE

	struct mxc_isi_ctx *mxc_ctx = priv;
	struct mxc_isi_m2m_dev *isi_m2m = mxc_ctx->isi_m2m;
	struct v4l2_fh *fh = &mxc_ctx->fh;
	unsigned int num_src_bufs_ready;
	unsigned int num_dst_bufs_ready;
	unsigned long flags;

	dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s\n", __func__);

	spin_lock_irqsave(&isi_m2m->slock, flags);
	num_src_bufs_ready = v4l2_m2m_num_src_bufs_ready(fh->m2m_ctx);
	num_dst_bufs_ready = v4l2_m2m_num_dst_bufs_ready(fh->m2m_ctx);
	spin_unlock_irqrestore(&isi_m2m->slock, flags);

	if (num_src_bufs_ready >= 1 && num_dst_bufs_ready >= 1)
		return 1;
	return 0;
}

static void mxc_isi_m2m_job_abort(void *priv)
{
	TRACE

	struct mxc_isi_ctx *mxc_ctx = priv;
	struct mxc_isi_m2m_dev *isi_m2m = mxc_ctx->isi_m2m;

	isi_m2m->aborting = 1;
	dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] Abort requested\n");
}

static struct v4l2_m2m_ops mxc_isi_m2m_ops = {
	.device_run = mxc_isi_m2m_device_run,
	.job_ready  = mxc_isi_m2m_job_ready,
	.job_abort  = mxc_isi_m2m_job_abort,
};

static int m2m_vb2_queue_setup(struct vb2_queue *q,
		unsigned int *num_buffers, unsigned int *num_planes,
		unsigned int sizes[], struct device *alloc_devs[])
{
	TRACE

	struct mxc_isi_ctx *mxc_ctx = vb2_get_drv_priv(q);
	struct mxc_isi_m2m_dev *isi_m2m = mxc_ctx->isi_m2m;
	struct device *dev = &isi_m2m->pdev->dev;
	struct mxc_isi_frame *frame;
	struct mxc_isi_fmt *fmt;
	unsigned long wh;
	int i;

	dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s\n", __func__);

	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (*num_buffers < 3) {
			dev_err(dev, "[vc-mipi isi-m2m] %s at least need 3 buffer\n", __func__);
			return -EINVAL;
		}
		frame = &isi_m2m->dst_f;
		isi_m2m->req_cap_buf_num = *num_buffers;
	} else {
		if (*num_buffers < 1) {
			dev_err(dev, "[vc-mipi isi-m2m] %s at least need one buffer\n", __func__);
			return -EINVAL;
		}
		frame = &isi_m2m->src_f;
		isi_m2m->req_out_buf_num = *num_buffers;
	}

	fmt = frame->fmt;
	if (fmt == NULL)
		return -EINVAL;

	for (i = 0; i < fmt->memplanes; i++)
		alloc_devs[i] = &isi_m2m->pdev->dev;

	*num_planes = fmt->memplanes;
	wh = frame->width * frame->height;

	for (i = 0; i < fmt->memplanes; i++) {
		unsigned int size = (wh * fmt->depth[i]) >> 3;

		if (i == 1 && fmt->fourcc == V4L2_PIX_FMT_NV12)
			size >>= 1;
		sizes[i] = max_t(u32, size, frame->sizeimage[i]);

		dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s, buf_n=%d, planes[%d]->size=%d\n",
					__func__,  *num_buffers, i, sizes[i]);
	}

	return 0;
}

static int m2m_vb2_buffer_prepare(struct vb2_buffer *vb2)
{
	TRACE

	struct vb2_queue *vq = vb2->vb2_queue;
	struct mxc_isi_ctx *mxc_ctx = vb2_get_drv_priv(vq);
	struct mxc_isi_m2m_dev *isi_m2m = mxc_ctx->isi_m2m;
	struct mxc_isi_frame *frame;
	int i;

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		frame = &isi_m2m->dst_f;
	else
		frame = &isi_m2m->src_f;

	if (frame == NULL)
		return -EINVAL;

	for (i = 0; i < frame->fmt->memplanes; i++) {
		unsigned long size = frame->sizeimage[i];

		if (vb2_plane_size(vb2, i) < size) {
			dev_err(&isi_m2m->pdev->dev,
				 "[vc-mipi isi-m2m] User buffer too small (%ld < %ld)\n",
				 vb2_plane_size(vb2, i), size);
			return -EINVAL;
		}
		vb2_set_plane_payload(vb2, i, size);
	}

	return 0;
}

static void m2m_vb2_buffer_queue(struct vb2_buffer *vb2)
{
	TRACE

	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb2);
	struct mxc_isi_ctx *mxc_ctx = vb2_get_drv_priv(vb2->vb2_queue);
	struct v4l2_fh *fh = &mxc_ctx->fh;

	v4l2_m2m_buf_queue(fh->m2m_ctx, vbuf);
}

static int m2m_vb2_start_streaming(struct vb2_queue *q, unsigned int count)
{
	TRACE

	struct mxc_isi_ctx *mxc_ctx = vb2_get_drv_priv(q);
	struct mxc_isi_m2m_dev *isi_m2m = mxc_ctx->isi_m2m;
	struct mxc_isi_dev *mxc_isi = mxc_isi_get_hostdata(isi_m2m->pdev);
	struct v4l2_fh *fh = &mxc_ctx->fh;
	struct vb2_v4l2_buffer *dst_vbuf;
	struct  v4l2_m2m_buffer *b;
	struct mxc_isi_buffer *dst_buf;
	unsigned long flags;

	if (V4L2_TYPE_IS_OUTPUT(q->type))
		return 0;

	if (count < 2) {
		dev_err(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] Need to at leas 2 buffers\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&isi_m2m->slock, flags);

	/* BUF1 */
	dst_vbuf = v4l2_m2m_next_dst_buf(fh->m2m_ctx);
	if (!dst_vbuf) {
		dev_err(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %d: Null dst buf\n", __LINE__);
		goto unlock;
	}
	dst_vbuf->vb2_buf.state = VB2_BUF_STATE_ACTIVE;
	dst_buf = to_isi_buffer(dst_vbuf);
	dst_buf->v4l2_buf.sequence = 0;
	mxc_isi_channel_set_outbuf(mxc_isi, dst_buf);
	v4l2_m2m_dst_buf_remove(fh->m2m_ctx);
	b = to_v4l2_m2m_buffer(dst_vbuf);
	list_add_tail(&b->list, &isi_m2m->out_active);

	/* BUF2 */
	dst_vbuf = v4l2_m2m_next_dst_buf(fh->m2m_ctx);
	if (!dst_vbuf) {
		dev_err(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %d: Null dst buf\n", __LINE__);
		goto unlock;
	}
	dst_vbuf->vb2_buf.state = VB2_BUF_STATE_ACTIVE;
	dst_buf = to_isi_buffer(dst_vbuf);
	dst_buf->v4l2_buf.sequence = 1;
	mxc_isi_channel_set_outbuf(mxc_isi, dst_buf);
	v4l2_m2m_dst_buf_remove(fh->m2m_ctx);
	b = to_v4l2_m2m_buffer(dst_vbuf);
	list_add_tail(&b->list, &isi_m2m->out_active);

	isi_m2m->frame_count = 1;
	isi_m2m->aborting = 0;
unlock:
	spin_unlock_irqrestore(&isi_m2m->slock, flags);

	return 0;
}

static void m2m_vb2_stop_streaming(struct vb2_queue *q)
{
	TRACE

	struct mxc_isi_ctx *mxc_ctx = vb2_get_drv_priv(q);
	struct mxc_isi_m2m_dev *isi_m2m = mxc_ctx->isi_m2m;
	struct vb2_v4l2_buffer *vb2;
	struct mxc_isi_buffer *buf;
	unsigned long flags;

	spin_lock_irqsave(&isi_m2m->slock, flags);

	while ((vb2 = v4l2_m2m_src_buf_remove(mxc_ctx->fh.m2m_ctx)) != NULL)
		v4l2_m2m_buf_done(vb2, VB2_BUF_STATE_ERROR);

	while ((vb2 = v4l2_m2m_dst_buf_remove(mxc_ctx->fh.m2m_ctx)) != NULL)
		v4l2_m2m_buf_done(vb2, VB2_BUF_STATE_ERROR);

	while (!list_empty(&isi_m2m->out_active)) {
		buf = list_entry(isi_m2m->out_active.next, struct mxc_isi_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->v4l2_buf.vb2_buf, VB2_BUF_STATE_ERROR);
	}

	INIT_LIST_HEAD(&isi_m2m->out_active);

	spin_unlock_irqrestore(&isi_m2m->slock, flags);
}

static struct vb2_ops mxc_m2m_vb2_qops = {
	.queue_setup		= m2m_vb2_queue_setup,
	.buf_prepare		= m2m_vb2_buffer_prepare,
	.buf_queue		= m2m_vb2_buffer_queue,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
	.start_streaming	= m2m_vb2_start_streaming,
	.stop_streaming		= m2m_vb2_stop_streaming,
};

static int mxc_m2m_queue_init(void *priv, struct vb2_queue *src_vq,
			       struct vb2_queue *dst_vq)
{
	TRACE

	struct mxc_isi_ctx *mxc_ctx = priv;
	struct mxc_isi_m2m_dev *isi_m2m = mxc_ctx->isi_m2m;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	src_vq->drv_priv = mxc_ctx;
	src_vq->buf_struct_size = sizeof(struct mxc_isi_buffer);
	src_vq->ops = &mxc_m2m_vb2_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &isi_m2m->lock;
	src_vq->dev = &isi_m2m->pdev->dev;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	dst_vq->drv_priv = mxc_ctx;
	dst_vq->buf_struct_size = sizeof(struct mxc_isi_buffer);
	dst_vq->ops = &mxc_m2m_vb2_qops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &isi_m2m->lock;
	dst_vq->dev = &isi_m2m->pdev->dev;

	ret = vb2_queue_init(dst_vq);
	return ret;
}

static int mxc_isi_m2m_open(struct file *file)
{
	TRACE

	struct video_device *vdev = video_devdata(file);
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);
	struct mxc_isi_dev *mxc_isi = mxc_isi_get_hostdata(isi_m2m->pdev);
	struct device *dev = &isi_m2m->pdev->dev;
	struct mxc_isi_ctx *mxc_ctx = NULL;
	int ret = 0;

	if (atomic_read(&mxc_isi->usage_count) > 0) {
		dev_err(dev, "[vc-mipi isi-m2m] ISI channel[%d] is busy\n", isi_m2m->id);
		return -EBUSY;
	}

	if (mutex_lock_interruptible(&isi_m2m->lock))
		return -ERESTARTSYS;

	mxc_ctx = kzalloc(sizeof(*mxc_ctx), GFP_KERNEL);
	if (!mxc_ctx) {
		ret = -ENOMEM;
		goto unlock;
	}

	mxc_ctx->isi_m2m = isi_m2m;

	v4l2_fh_init(&mxc_ctx->fh, vdev);
	file->private_data = &mxc_ctx->fh;

	mxc_ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(isi_m2m->m2m_dev,
						mxc_ctx,
						mxc_m2m_queue_init);
	if (IS_ERR(mxc_ctx->fh.m2m_ctx)) {
		dev_err(dev, "[vc-mipi isi-m2m] v4l2_m2m_ctx_init fail\n");
		ret = PTR_ERR(mxc_ctx->fh.m2m_ctx);
		v4l2_fh_exit(&mxc_ctx->fh);
		kfree(mxc_ctx);
		goto unlock;
	}
	v4l2_fh_add(&mxc_ctx->fh);

	pm_runtime_get_sync(dev);
	if (atomic_inc_return(&mxc_isi->usage_count) == 1)
		mxc_isi_channel_init(mxc_isi);

	/* lock host data */
	mutex_lock(&mxc_isi->lock);
	mxc_isi->m2m_enabled = true;
	mutex_unlock(&mxc_isi->lock);
unlock:
	mutex_unlock(&isi_m2m->lock);
	return ret;
}

static int mxc_isi_m2m_release(struct file *file)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);
	struct mxc_isi_dev *mxc_isi = mxc_isi_get_hostdata(isi_m2m->pdev);
	struct device *dev = &isi_m2m->pdev->dev;
	struct mxc_isi_ctx *mxc_ctx = file_to_ctx(file);

	v4l2_fh_del(&mxc_ctx->fh);
	v4l2_fh_exit(&mxc_ctx->fh);

	mutex_lock(&isi_m2m->lock);
	v4l2_m2m_ctx_release(mxc_ctx->fh.m2m_ctx);
	mutex_unlock(&isi_m2m->lock);

	kfree(mxc_ctx);
	if (atomic_dec_and_test(&mxc_isi->usage_count))
		mxc_isi_channel_deinit(mxc_isi);

	mutex_lock(&mxc_isi->lock);
	mxc_isi->m2m_enabled = false;
	mutex_unlock(&mxc_isi->lock);

	pm_runtime_put(dev);
	return 0;
}

static const struct v4l2_file_operations mxc_isi_m2m_fops = {
	.owner			= THIS_MODULE,
	.open			= mxc_isi_m2m_open,
	.release		= mxc_isi_m2m_release,
	.poll			= v4l2_m2m_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap			= v4l2_m2m_fop_mmap,
};

static int mxc_isi_m2m_querycap(struct file *file, void *priv,
					struct v4l2_capability *cap)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);

	strlcpy(cap->driver, MXC_ISI_M2M, sizeof(cap->driver));
	strlcpy(cap->card, MXC_ISI_M2M, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s.%d",
		 dev_name(&isi_m2m->pdev->dev), isi_m2m->id);
	cap->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_M2M_MPLANE;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int mxc_isi_m2m_enum_fmt_vid_out(struct file *file, void *priv,
				    struct v4l2_fmtdesc *f)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);
	struct mxc_isi_fmt *fmt;

	dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s\n", __func__);
	if (f->index >= (int)ARRAY_SIZE(mxc_isi_input_formats))
		return -EINVAL;

	fmt = &mxc_isi_input_formats[f->index];
	strncpy(f->description, fmt->name, sizeof(f->description) - 1);

	f->pixelformat = fmt->fourcc;

	return 0;
}

static int mxc_isi_m2m_enum_fmt_vid_cap(struct file *file, void *priv,
				    struct v4l2_fmtdesc *f)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);
	struct mxc_isi_fmt *fmt;

	dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s\n", __func__);
	if (f->index >= (int)ARRAY_SIZE(mxc_isi_out_formats))
		return -EINVAL;

	fmt = &mxc_isi_out_formats[f->index];
	strncpy(f->description, fmt->name, sizeof(f->description) - 1);

	f->pixelformat = fmt->fourcc;

	return 0;
}

static int mxc_isi_m2m_try_fmt_vid_out(struct file *file, void *fh,
				   struct v4l2_format *f)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);
	struct device *dev = &isi_m2m->pdev->dev;
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;
	struct mxc_isi_fmt *fmt = NULL;
	int i;

	if (f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(mxc_isi_input_formats); i++) {
		fmt = &mxc_isi_input_formats[i];
		if (fmt->fourcc == pix->pixelformat)
			break;
	}

	if (i >= ARRAY_SIZE(mxc_isi_input_formats)) {
		dev_err(dev, "[vc-mipi isi-m2m] %s, format is not support!\n", __func__);
		return -EINVAL;
	}

	if (pix->width <= 0 || pix->height <= 0) {
		dev_err(dev, "[vc-mipi isi-m2m] %s, width %d, height %d is not valid\n"
				, __func__, pix->width, pix->height);
		return -EINVAL;
	}

	return 0;
}

static int mxc_isi_m2m_try_fmt_vid_cap(struct file *file, void *fh,
				   struct v4l2_format *f)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);
	struct device *dev = &isi_m2m->pdev->dev;
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;
	struct mxc_isi_fmt *fmt = NULL;
	int i;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(mxc_isi_out_formats); i++) {
		fmt = &mxc_isi_out_formats[i];
		if (fmt->fourcc == pix->pixelformat)
			break;
	}

	if (i >= ARRAY_SIZE(mxc_isi_out_formats)) {
		dev_err(dev, "[vc-mipi isi-m2m] %s, format is not support!\n", __func__);
		return -EINVAL;
	}

	if (pix->width <= 0 || pix->height <= 0) {
		dev_err(dev, "[vc-mipi isi-m2m] %s, width %d, height %d is not valid\n"
				, __func__, pix->width, pix->height);
		return -EINVAL;
	}

	return 0;
}

static int mxc_isi_m2m_s_fmt_vid_out(struct file *file, void *priv,
				 struct v4l2_format *f)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);
	struct mxc_isi_dev *mxc_isi = mxc_isi_get_hostdata(isi_m2m->pdev);
	struct v4l2_fh *fh = file->private_data;
	struct mxc_isi_frame *frame = &isi_m2m->src_f;
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;
	struct mxc_isi_fmt *fmt;
	struct vb2_queue *vq;
	int bpl, i;

	dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s\n", __func__);

	if (f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return -EINVAL;

	vq = v4l2_m2m_get_vq(fh->m2m_ctx, f->type);
	if (!vq)
		return -EINVAL;

	if (vb2_is_busy(vq)) {
		dev_err(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] queue busy\n");
		return -EBUSY;
	}

	for (i = 0; i < ARRAY_SIZE(mxc_isi_input_formats); i++) {
		fmt = &mxc_isi_input_formats[i];
		if (pix && fmt->fourcc == pix->pixelformat)
			break;
	}

	if (i >= ARRAY_SIZE(mxc_isi_input_formats)) {
		dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s, format is not support!\n", __func__);
		return -EINVAL;
	}

	/* update out put frame size and formate */
	if (pix->height <= 0 || pix->width <= 0)
		return -EINVAL;

	frame->fmt = fmt;
	frame->height = pix->height;
	frame->width = pix->width;

	pix->num_planes = fmt->memplanes;
	for (i = 0; i < pix->num_planes; i++) {
		bpl = pix->plane_fmt[i].bytesperline;

		if ((bpl == 0) || (bpl / (fmt->depth[i] >> 3)) < pix->width)
			pix->plane_fmt[i].bytesperline =
						(pix->width * fmt->depth[i]) >> 3;

		if (pix->plane_fmt[i].sizeimage == 0)
			pix->plane_fmt[i].sizeimage = (pix->width * pix->height *
						fmt->depth[i] >> 3);
	}

	frame->bytesperline[0] = frame->width * frame->fmt->depth[0] / 8;
	frame->sizeimage[0] = frame->height * frame->bytesperline[0];

	set_frame_bounds(frame, pix->width, pix->height);
	mxc_isi_m2m_config_src(mxc_isi, frame);

	return 0;
}

static int mxc_isi_m2m_s_fmt_vid_cap(struct file *file, void *priv,
				 struct v4l2_format *f)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);
	struct mxc_isi_dev *mxc_isi = mxc_isi_get_hostdata(isi_m2m->pdev);
	struct v4l2_fh *fh = file->private_data;
	struct mxc_isi_frame *frame = &isi_m2m->dst_f;
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;
	struct mxc_isi_fmt *fmt;
	struct vb2_queue *vq;
	int bpl, i;

	dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s\n", __func__);

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;

	vq = v4l2_m2m_get_vq(fh->m2m_ctx, f->type);
	if (!vq)
		return -EINVAL;

	if (vb2_is_busy(vq)) {
		dev_err(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] queue busy\n");
		return -EBUSY;
	}

	for (i = 0; i < ARRAY_SIZE(mxc_isi_out_formats); i++) {
		fmt = &mxc_isi_out_formats[i];
		if (pix && fmt->fourcc == pix->pixelformat)
			break;
	}

	if (i >= ARRAY_SIZE(mxc_isi_out_formats)) {
		dev_err(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s, format is not support!\n", __func__);
		return -EINVAL;
	}

	/* update out put frame size and formate */
	if (pix->height <= 0 || pix->width <= 0) {
		dev_err(&isi_m2m->pdev->dev,
			"[vc-mipi isi-m2m] Invalid width or height(w=%d, h=%d)\n",
			pix->width, pix->height);
		return -EINVAL;
	}

	if ((pix->pixelformat == V4L2_PIX_FMT_NV12) && ((pix->width / 4) % 2)) {
		dev_err(&isi_m2m->pdev->dev,
			"[vc-mipi isi-m2m] Invalid width or height(w=%d, h=%d) for NV12\n",
			pix->width, pix->height);
		return -EINVAL;
	} else if ((pix->pixelformat != V4L2_PIX_FMT_XBGR32) && (pix->width % 2)) {
		dev_err(&isi_m2m->pdev->dev,
			"[vc-mipi isi-m2m] Invalid width or height(w=%d, h=%d) for %.4s\n",
			pix->width, pix->height, (char *)&pix->pixelformat);
		return -EINVAL;
	}

	frame->fmt = fmt;
	frame->height = pix->height;
	frame->width = pix->width;

	pix->num_planes = fmt->memplanes;
	for (i = 0; i < pix->num_planes; i++) {
		bpl = pix->plane_fmt[i].bytesperline;

		if ((bpl == 0) || (bpl / (fmt->depth[i] >> 3)) < pix->width)
			pix->plane_fmt[i].bytesperline =
						(pix->width * fmt->depth[i]) >> 3;

		if (pix->plane_fmt[i].sizeimage == 0) {

			if ((i == 1) && (pix->pixelformat == V4L2_PIX_FMT_NV12))
				pix->plane_fmt[i].sizeimage =
					(pix->width * (pix->height >> 1) * fmt->depth[i] >> 3);
			else
				pix->plane_fmt[i].sizeimage = (pix->width * pix->height *
						fmt->depth[i] >> 3);
		}
	}

	if (pix->num_planes > 1) {
		for (i = 0; i < pix->num_planes; i++) {
			frame->bytesperline[i] = pix->plane_fmt[i].bytesperline;
			frame->sizeimage[i] = pix->plane_fmt[i].sizeimage;
		}
	} else {
		frame->bytesperline[0] = frame->width * frame->fmt->depth[0] / 8;
		frame->sizeimage[0] = frame->height * frame->bytesperline[0];
	}

	/*memcpy(&isi_m2m->pix, pix, sizeof(*pix));*/
	memcpy(&isi_m2m->pix, pix, sizeof(*pix));

	set_frame_bounds(frame, pix->width, pix->height);
	mxc_isi_m2m_config_dst(mxc_isi, frame);

	return 0;
}

static int mxc_isi_m2m_g_fmt_vid_cap(struct file *file, void *fh,
				 struct v4l2_format *f)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;
	struct mxc_isi_frame *frame = &isi_m2m->dst_f;
	int i;

	dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s\n", __func__);

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;

	pix->width = frame->o_width;
	pix->height = frame->o_height;
	pix->field = V4L2_FIELD_NONE;
	pix->pixelformat = frame->fmt->fourcc;
	pix->colorspace = V4L2_COLORSPACE_JPEG;
	pix->num_planes = frame->fmt->memplanes;

	for (i = 0; i < pix->num_planes; ++i) {
		pix->plane_fmt[i].bytesperline = frame->bytesperline[i];
		pix->plane_fmt[i].sizeimage = frame->sizeimage[i];
	}

	return 0;
}

static int mxc_isi_m2m_g_fmt_vid_out(struct file *file, void *fh,
				 struct v4l2_format *f)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;
	struct mxc_isi_frame *frame = &isi_m2m->src_f;
	int i;

	dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s\n", __func__);

	if (f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return -EINVAL;

	pix->width = frame->o_width;
	pix->height = frame->o_height;
	pix->field = V4L2_FIELD_NONE;
	pix->pixelformat = frame->fmt->fourcc;
	pix->colorspace = V4L2_COLORSPACE_JPEG;
	pix->num_planes = frame->fmt->memplanes;

	for (i = 0; i < pix->num_planes; ++i) {
		pix->plane_fmt[i].bytesperline = frame->bytesperline[i];
		pix->plane_fmt[i].sizeimage = frame->sizeimage[i];
	}

	return 0;
}

static int mxc_isi_m2m_streamon(struct file *file, void *priv,
			     enum v4l2_buf_type type)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);
	struct mxc_isi_dev *mxc_isi = mxc_isi_get_hostdata(isi_m2m->pdev);
	struct mxc_isi_frame *src_f, *dst_f;
	int ret;

	src_f = &isi_m2m->src_f;
	dst_f = &isi_m2m->dst_f;

	if ((dst_f->width  > src_f->width) ||
		(dst_f->height > src_f->height)) {
		dev_err(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s Not support upscale\n", __func__);
		return -EINVAL;
	}

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		isi_m2m->frame_count = 0;
		mxc_isi_channel_config(mxc_isi, src_f, dst_f);
	}

	ret = v4l2_m2m_ioctl_streamon(file, priv, type);

	return ret;
}

static int mxc_isi_m2m_streamoff(struct file *file, void *priv,
			    enum v4l2_buf_type type)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = video_drvdata(file);
	struct mxc_isi_dev *mxc_isi =  mxc_isi_get_hostdata(isi_m2m->pdev);
	int ret;

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		mxc_isi_channel_disable(mxc_isi);

	ret = v4l2_m2m_ioctl_streamoff(file, priv, type);

	return ret;
}

static const struct v4l2_ioctl_ops mxc_isi_m2m_ioctl_ops = {
	.vidioc_querycap		= mxc_isi_m2m_querycap,

	.vidioc_enum_fmt_vid_cap = mxc_isi_m2m_enum_fmt_vid_cap,
	.vidioc_enum_fmt_vid_out = mxc_isi_m2m_enum_fmt_vid_out,

	.vidioc_try_fmt_vid_cap_mplane = mxc_isi_m2m_try_fmt_vid_cap,
	.vidioc_try_fmt_vid_out_mplane = mxc_isi_m2m_try_fmt_vid_out,

	.vidioc_s_fmt_vid_cap_mplane = mxc_isi_m2m_s_fmt_vid_cap,
	.vidioc_s_fmt_vid_out_mplane = mxc_isi_m2m_s_fmt_vid_out,

	.vidioc_g_fmt_vid_cap_mplane = mxc_isi_m2m_g_fmt_vid_cap,
	.vidioc_g_fmt_vid_out_mplane = mxc_isi_m2m_g_fmt_vid_out,

	.vidioc_reqbufs		= v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf	= v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf		= v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf		= v4l2_m2m_ioctl_dqbuf,
	.vidioc_expbuf		= v4l2_m2m_ioctl_expbuf,
	.vidioc_prepare_buf	= v4l2_m2m_ioctl_prepare_buf,
	.vidioc_create_bufs	= v4l2_m2m_ioctl_create_bufs,

	.vidioc_streamon	= mxc_isi_m2m_streamon,
	.vidioc_streamoff	= mxc_isi_m2m_streamoff,
};

/*
 * V4L2 controls handling
 */
#define ctrl_to_mxc_isi_m2m(__ctrl) \
	container_of((__ctrl)->handler, struct mxc_isi_m2m_dev, ctrls.handler)

static int mxc_isi_m2m_s_ctrl(struct v4l2_ctrl *ctrl)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = ctrl_to_mxc_isi_m2m(ctrl);
	struct mxc_isi_dev *mxc_isi = mxc_isi_get_hostdata(isi_m2m->pdev);
	unsigned long flags;
	int ret = 0;

	dev_dbg(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s\n", __func__);

	if (ctrl->flags & V4L2_CTRL_FLAG_INACTIVE)
		return 0;

	spin_lock_irqsave(&mxc_isi->slock, flags);

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		if (ctrl->val < 0) {
			ret = -EINVAL;
			goto unlock;
		}
		mxc_isi->hflip = (ctrl->val > 0) ? 1 : 0;
		break;

	case V4L2_CID_VFLIP:
		if (ctrl->val < 0) {
			ret = -EINVAL;
			goto unlock;
		}
		mxc_isi->vflip = (ctrl->val > 0) ? 1 : 0;
		break;

	case V4L2_CID_ALPHA_COMPONENT:
		if (ctrl->val < 0 || ctrl->val > 255) {
			ret = -EINVAL;
			goto unlock;
		}
		mxc_isi->alpha = ctrl->val;
		mxc_isi->alphaen = 1;
		break;

	default:
		dev_err(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s: Not support %d CID\n", __func__, ctrl->id);
		ret = -EINVAL;
	}

unlock:
	spin_unlock_irqrestore(&mxc_isi->slock, flags);
	return ret;
}

static int mxc_isi_m2m_g_ctrl(struct v4l2_ctrl *ctrl)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = ctrl_to_mxc_isi_m2m(ctrl);
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&isi_m2m->slock, flags);

	switch (ctrl->id) {
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
		ctrl->val = isi_m2m->req_cap_buf_num;
		break;
	case V4L2_CID_MIN_BUFFERS_FOR_OUTPUT:
		ctrl->val = isi_m2m->req_out_buf_num;
		break;
	default:
		dev_err(&isi_m2m->pdev->dev, "[vc-mipi isi-m2m] %s: Not support %d CID\n",
					__func__, ctrl->id);
		ret = -EINVAL;
	}

	spin_unlock_irqrestore(&isi_m2m->slock, flags);
	return ret;

}

static const struct v4l2_ctrl_ops mxc_isi_m2m_ctrl_ops = {
	.s_ctrl = mxc_isi_m2m_s_ctrl,
	.g_volatile_ctrl = mxc_isi_m2m_g_ctrl,
};

static int mxc_isi_m2m_ctrls_create(struct mxc_isi_m2m_dev *isi_m2m)
{
	TRACE
	
	struct mxc_isi_ctrls *ctrls = &isi_m2m->ctrls;
	struct v4l2_ctrl_handler *handler = &ctrls->handler;

	if (isi_m2m->ctrls.ready)
		return 0;

	v4l2_ctrl_handler_init(handler, 4);

	ctrls->hflip = v4l2_ctrl_new_std(handler, &mxc_isi_m2m_ctrl_ops,
					V4L2_CID_HFLIP, 0, 1, 1, 0);
	ctrls->vflip = v4l2_ctrl_new_std(handler, &mxc_isi_m2m_ctrl_ops,
					V4L2_CID_VFLIP, 0, 1, 1, 0);
	ctrls->alpha = v4l2_ctrl_new_std(handler, &mxc_isi_m2m_ctrl_ops,
					V4L2_CID_ALPHA_COMPONENT, 0, 0xff, 1, 0);
	ctrls->num_cap_buf = v4l2_ctrl_new_std(handler, &mxc_isi_m2m_ctrl_ops,
					V4L2_CID_MIN_BUFFERS_FOR_CAPTURE, 3, 16, 1, 3);
	ctrls->num_out_buf = v4l2_ctrl_new_std(handler, &mxc_isi_m2m_ctrl_ops,
					V4L2_CID_MIN_BUFFERS_FOR_OUTPUT, 1, 16, 1, 1);

	if (!handler->error)
		ctrls->ready = true;

	return handler->error;

}

void mxc_isi_m2m_ctrls_delete(struct mxc_isi_m2m_dev *isi_m2m)
{
	TRACE
	
	struct mxc_isi_ctrls *ctrls = &isi_m2m->ctrls;

	if (ctrls->ready) {
		v4l2_ctrl_handler_free(&ctrls->handler);
		ctrls->ready = false;
		ctrls->alpha = NULL;
	}
}

static int isi_m2m_probe(struct platform_device *pdev)
{
	TRACE
	
	struct mxc_isi_dev *mxc_isi;
	struct mxc_isi_m2m_dev *isi_m2m;
	struct v4l2_device *v4l2_dev;
	struct video_device *vdev;
	int ret = -ENOMEM;

	isi_m2m = devm_kzalloc(&pdev->dev, sizeof(*isi_m2m), GFP_KERNEL);
	if (!isi_m2m)
		return -ENOMEM;
	isi_m2m->pdev = pdev;

	pdev->dev.parent = mxc_isi_dev_get_parent(pdev);
	if (!pdev->dev.parent) {
		dev_info(&pdev->dev, "[vc-mipi isi-m2m] deferring %s device registration\n",
			 dev_name(&pdev->dev));
		return -EPROBE_DEFER;
	}

	mxc_isi = mxc_isi_get_hostdata(pdev);
	if (!mxc_isi) {
		dev_info(&pdev->dev, "[vc-mipi isi-m2m] deferring %s device registration\n",
			 dev_name(&pdev->dev));
		return -EPROBE_DEFER;
	}
	mxc_isi->isi_m2m = isi_m2m;
	isi_m2m->id = mxc_isi->id;

	spin_lock_init(&isi_m2m->slock);
	mutex_init(&isi_m2m->lock);

	/* m2m */
	isi_m2m->m2m_dev = v4l2_m2m_init(&mxc_isi_m2m_ops);
	if (IS_ERR(isi_m2m->m2m_dev)) {
		dev_err(&pdev->dev, "[vc-mipi isi-m2m] %s fail to get m2m device\n", __func__);
		return PTR_ERR(isi_m2m->m2m_dev);
	}

	/* V4L2 device */
	v4l2_dev = &isi_m2m->v4l2_dev;
	strlcpy(v4l2_dev->name, "mx8-isi-m2m", sizeof(v4l2_dev->name));

	ret = v4l2_device_register(&pdev->dev, v4l2_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "[vc-mipi isi-m2m] Failed to register v4l2_device\n");
		return -EINVAL;
	}

	INIT_LIST_HEAD(&isi_m2m->out_active);

	/* Video device */
	vdev = &isi_m2m->vdev;
	memset(vdev, 0, sizeof(*vdev));
	snprintf(vdev->name, sizeof(vdev->name), "mxc_isi.%d.m2m", isi_m2m->id);

	vdev->fops	= &mxc_isi_m2m_fops;
	vdev->ioctl_ops	= &mxc_isi_m2m_ioctl_ops;
	vdev->v4l2_dev	= v4l2_dev;
	vdev->minor	= -1;
	vdev->release	= video_device_release_empty;
	vdev->vfl_dir = VFL_DIR_M2M;
	vdev->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_M2M_MPLANE;

	ret = mxc_isi_m2m_ctrls_create(isi_m2m);
	if (ret)
		goto free_m2m;

	ret = video_register_device(vdev, VFL_TYPE_GRABBER, -1);
	if (ret < 0) {
		dev_err(&pdev->dev, "[vc-mipi isi-m2m] %s fail to register video device\n", __func__);
		goto ctrl_free;
	}

	vdev->ctrl_handler = &isi_m2m->ctrls.handler;
	video_set_drvdata(vdev, isi_m2m);
	platform_set_drvdata(pdev, isi_m2m);
	pm_runtime_enable(&pdev->dev);

	dev_info(&pdev->dev, "[vc-mipi isi-m2m] Register m2m success for ISI.%d\n", isi_m2m->id);

	return 0;

ctrl_free:
	mxc_isi_m2m_ctrls_delete(isi_m2m);
free_m2m:
	v4l2_m2m_release(isi_m2m->m2m_dev);
	return ret;

}

static int isi_m2m_remove(struct platform_device *pdev)
{
	TRACE
	
	struct mxc_isi_m2m_dev *isi_m2m = platform_get_drvdata(pdev);
	struct video_device *vdev = &isi_m2m->vdev;

	if (video_is_registered(vdev)) {
		video_unregister_device(vdev);
		mxc_isi_m2m_ctrls_delete(isi_m2m);
		media_entity_cleanup(&vdev->entity);
	}
	v4l2_m2m_release(isi_m2m->m2m_dev);
	v4l2_device_unregister(&isi_m2m->v4l2_dev);
	pm_runtime_disable(&isi_m2m->pdev->dev);

	return 0;
}

static const struct of_device_id isi_m2m_of_match[] = {
	{.compatible = "imx-isi-m2m",},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, isi_m2m_of_match);

static struct platform_driver isi_m2m_driver = {
	.probe  = isi_m2m_probe,
	.remove = isi_m2m_remove,
	.driver = {
		.of_match_table = isi_m2m_of_match,
		.name		= "isi-m2m",
	},
};

static int __init mxc_isi_m2m_init(void)
{
	return platform_driver_register(&isi_m2m_driver);
}
late_initcall(mxc_isi_m2m_init);

static void __exit mxc_isi_m2m_exit(void)
{
	platform_driver_unregister(&isi_m2m_driver);
}
module_exit(mxc_isi_m2m_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("IMX8 Image Sensor Interface memory to memory driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ISI M2M");
MODULE_VERSION("1.0");
