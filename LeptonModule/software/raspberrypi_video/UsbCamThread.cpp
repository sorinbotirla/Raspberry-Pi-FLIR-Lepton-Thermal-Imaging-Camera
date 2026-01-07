#include "UsbCamThread.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include <cstring>
#include <cstdio>
#include <algorithm>

static inline uint8_t clamp8(int v) { return (uint8_t)std::min(255, std::max(0, v)); }

// fast-ish YUYV -> RGB565
static inline uint16_t yuv_to_rgb565(int y, int u, int v)
{
    int c = y - 16;
    int d = u - 128;
    int e = v - 128;

    int r = (298 * c + 409 * e + 128) >> 8;
    int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
    int b = (298 * c + 516 * d + 128) >> 8;

    uint8_t R = clamp8(r);
    uint8_t G = clamp8(g);
    uint8_t B = clamp8(b);

    return (uint16_t)(((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3));
}

UsbCamThread::UsbCamThread(const QString &device, QObject *parent)
    : QThread(parent), m_dev(device)
{
}

UsbCamThread::~UsbCamThread()
{
    m_stop = true;
    wait(1000);
}

void UsbCamThread::setSize(int w, int h)
{
    m_w = w;
    m_h = h;
}

void UsbCamThread::setFps(int fps)
{
    m_fps = fps;
}

void UsbCamThread::run()
{
    int fd = open(m_dev.toUtf8().constData(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) return;

    // set format
    v4l2_format fmt;
    std::memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = m_w;
    fmt.fmt.pix.height = m_h;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        close(fd);
        return;
    }

    // try set fps
    v4l2_streamparm sp;
    std::memset(&sp, 0, sizeof(sp));
    sp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_PARM, &sp) == 0) {
        if (sp.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
            sp.parm.capture.timeperframe.numerator = 1;
            sp.parm.capture.timeperframe.denominator = std::max(1, m_fps);
            ioctl(fd, VIDIOC_S_PARM, &sp);
        }
    }

    // request buffers
    v4l2_requestbuffers req;
    std::memset(&req, 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 2) {
        close(fd);
        return;
    }

    struct Buf { void *ptr; size_t len; };
    Buf bufs[8];
    int nbufs = std::min<int>(req.count, 8);

    for (int i = 0; i < nbufs; i++) {
        v4l2_buffer b;
        std::memset(&b, 0, sizeof(b));
        b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        b.memory = V4L2_MEMORY_MMAP;
        b.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &b) < 0) {
            close(fd);
            return;
        }
        bufs[i].len = b.length;
        bufs[i].ptr = mmap(nullptr, b.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, b.m.offset);
        if (bufs[i].ptr == MAP_FAILED) {
            close(fd);
            return;
        }
    }

    // queue all
    for (int i = 0; i < nbufs; i++) {
        v4l2_buffer b;
        std::memset(&b, 0, sizeof(b));
        b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        b.memory = V4L2_MEMORY_MMAP;
        b.index = i;
        ioctl(fd, VIDIOC_QBUF, &b);
    }

    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        close(fd);
        return;
    }

    QImage frame(m_w, m_h, QImage::Format_RGB16);

    while (!m_stop) {
        v4l2_buffer b;
        std::memset(&b, 0, sizeof(b));
        b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        b.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd, VIDIOC_DQBUF, &b) < 0) {
            usleep(2000);
            continue;
        }

        const uint8_t *src = (const uint8_t*)bufs[b.index].ptr;
        uint16_t *dst = (uint16_t*)frame.bits();

        // YUYV: Y0 U Y1 V
        int pixels = m_w * m_h;
        for (int i = 0; i < pixels; i += 2) {
            int y0 = src[0];
            int u  = src[1];
            int y1 = src[2];
            int v  = src[3];
            src += 4;

            dst[i]     = yuv_to_rgb565(y0, u, v);
            dst[i + 1] = yuv_to_rgb565(y1, u, v);
        }

        emit updateCamera(frame);

        ioctl(fd, VIDIOC_QBUF, &b);
    }

    ioctl(fd, VIDIOC_STREAMOFF, &type);

    for (int i = 0; i < nbufs; i++) {
        if (bufs[i].ptr && bufs[i].ptr != MAP_FAILED) munmap(bufs[i].ptr, bufs[i].len);
    }

    close(fd);
}
