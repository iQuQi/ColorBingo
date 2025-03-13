#include "v4l2camera.h"
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

V4L2Camera::V4L2Camera(QObject *parent) :
    QObject(parent),
    fd(-1),
    buffers(NULL),
    n_buffers(0),
    isCapturing(false),
    stopThread(false)
{
}

V4L2Camera::~V4L2Camera()
{
    closeCamera();
}

bool V4L2Camera::openCamera(const QString &deviceName)
{
    struct stat st;
    
    devicePath = deviceName;

    if (stat(deviceName.toLocal8Bit().constData(), &st) == -1) {
        qDebug() << "Cannot identify device:" << deviceName << strerror(errno);
        return false;
    }

    if (!S_ISCHR(st.st_mode)) {
        qDebug() << deviceName << "is not a device";
        return false;
    }

    fd = open(deviceName.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK, 0);

    if (fd == -1) {
        qDebug() << "Cannot open" << deviceName << ":" << strerror(errno);
        return false;
    }

    initDevice();
    return true;
}

void V4L2Camera::closeCamera()
{
    if (isCapturing) {
        stopCapturing();
    }

    if (fd != -1) {
        uninitDevice();
        close(fd);
        fd = -1;
    }
}

void V4L2Camera::initDevice()
{
    struct v4l2_capability cap;

    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        if (EINVAL == errno) {
            qDebug() << "Device is not a V4L2 device";
            return;
        } else {
            qDebug() << "VIDIOC_QUERYCAP error:" << strerror(errno);
            return;
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        qDebug() << "Device is not a video capture device";
        return;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        qDebug() << "Device does not support streaming I/O";
        return;
    }

    // Set video format
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
        qDebug() << "VIDIOC_S_FMT error:" << strerror(errno);
        return;
    }
    
    // Set frame rate (FPS improvement)
    struct v4l2_streamparm streamparm;
    CLEAR(streamparm);
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    // Get current settings
    if (xioctl(fd, VIDIOC_G_PARM, &streamparm) == -1) {
        qDebug() << "VIDIOC_G_PARM error:" << strerror(errno);
    } 
    else if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
        // Set frame rate to 30fps (time interval = 1second/fps)
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = 30;
        
        if (xioctl(fd, VIDIOC_S_PARM, &streamparm) == -1) {
            qDebug() << "VIDIOC_S_PARM error:" << strerror(errno);
        }
        
        qDebug() << "Camera FPS set to:" << streamparm.parm.capture.timeperframe.denominator 
                 << "/" << streamparm.parm.capture.timeperframe.numerator;
    }

    initMMAP();
}

void V4L2Camera::initMMAP()
{
    struct v4l2_requestbuffers req;

    CLEAR(req);
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        if (EINVAL == errno) {
            qDebug() << "Device does not support memory mapping";
            return;
        } else {
            qDebug() << "VIDIOC_REQBUFS error:" << strerror(errno);
            return;
        }
    }

    if (req.count < 2) {
        qDebug() << "Insufficient buffer memory";
        return;
    }

    buffers = (struct Buffer*)calloc(req.count, sizeof(*buffers));

    if (!buffers) {
        qDebug() << "Out of memory";
        return;
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            qDebug() << "VIDIOC_QUERYBUF error:" << strerror(errno);
            return;
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL, buf.length,
              PROT_READ | PROT_WRITE, MAP_SHARED,
              fd, buf.m.offset);

        if (buffers[n_buffers].start == MAP_FAILED) {
            qDebug() << "mmap error:" << strerror(errno);
            return;
        }
    }
}

void V4L2Camera::uninitDevice()
{
    unsigned int i;

    for (i = 0; i < n_buffers; ++i) {
        if (munmap(buffers[i].start, buffers[i].length) == -1) {
            qDebug() << "munmap error";
        }
    }

    free(buffers);
    buffers = NULL;
}

bool V4L2Camera::startCapturing()
{
    if (isCapturing)
        return true;
        
    if (fd == -1) {
        // 장치가 닫혀있으면 다시 열기 시도
        if (!openCamera(devicePath)) {
            return false;
        }
    }

    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            qDebug() << "VIDIOC_QBUF error:" << strerror(errno);
            return false;
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        qDebug() << "VIDIOC_STREAMON error:" << strerror(errno);
        return false;
    }

    // Create a new image with the right dimensions
    currentFrame = QImage(fmt.fmt.pix.width, fmt.fmt.pix.height, QImage::Format_RGB888);
    currentFrame.fill(Qt::black);

    // Start capture thread
    stopThread = false;
    isCapturing = true;
    if (pthread_create(&captureThread, NULL, captureThreadFunc, this) != 0) {
        qDebug() << "Failed to create capture thread";
        isCapturing = false;
        return false;
    }

    return true;
}

void V4L2Camera::stopCapturing()
{
    if (!isCapturing)
        return;

    // Signal thread to stop and wait for it
    stopThread = true;
    pthread_join(captureThread, NULL);

    // Stop streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
        qDebug() << "VIDIOC_STREAMOFF error:" << strerror(errno);
    }

    isCapturing = false;
}

void* V4L2Camera::captureThreadFunc(void *arg)
{
    V4L2Camera *camera = static_cast<V4L2Camera*>(arg);
    camera->captureThreadLoop();
    return NULL;
}

void V4L2Camera::captureThreadLoop()
{
    fd_set fds;
    struct timeval tv;
    int r;
    int errorCount = 0;  // Track consecutive errors

    while (!stopThread) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        // Timeout
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (r == -1) {
            if (errno == EINTR)
                continue;
            qDebug() << "select error:" << strerror(errno);
            
            // Error detection and handling
            errorCount++;
            if (errorCount > 3) {  // More than 3 consecutive errors
                emit deviceDisconnected();
                qDebug() << "Device connection lost detected";
                break;
            }
            
            continue;
        }

        errorCount = 0;  // Reset counter if no errors

        if (r == 0) {
            // Timeout, no data available
            continue;
        }

        if (readFrame()) {
            emit newFrameAvailable();
        }
    }
}

bool V4L2Camera::readFrame()
{
    struct v4l2_buffer buf;

    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        switch (errno) {
        case EAGAIN:
            return false;
        case EIO:
            // Could ignore EIO, see spec
            // fall through
        case ENODEV:  // Device not found error
            qDebug() << "Camera device error: " << strerror(errno);
            emit deviceDisconnected();
            return false;
        default:
            qDebug() << "VIDIOC_DQBUF error:" << strerror(errno);
            return false;
        }
    }

    if (buf.index >= n_buffers) {
        qDebug() << "Buffer index out of range";
        return false;
    }

    processImage(buffers[buf.index].start, 0);

    if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
        qDebug() << "VIDIOC_QBUF error:" << strerror(errno);
        return false;
    }

    return true;
}

void V4L2Camera::processImage(const void *p, int /* size */)
{
    frameMutex.lock();
    yuv422ToRgb888(p, currentFrame);
    frameMutex.unlock();
}

void V4L2Camera::yuv422ToRgb888(const void *yuv, QImage &rgbImage)
{
    // 이미지 크기 가져오기
    int width = fmt.fmt.pix.width;
    int height = fmt.fmt.pix.height;
    
    // YUV 데이터를 RGB로 변환
    unsigned char *pp = (unsigned char *)yuv;
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 2; j++) {
            // YUV422 포맷에서 2개의 픽셀이 4바이트로 저장됨 (Y1 U Y2 V)
            unsigned char Y0 = pp[(i * width + j * 2) * 2];
            unsigned char U  = pp[(i * width + j * 2) * 2 + 1];
            unsigned char Y1 = pp[(i * width + j * 2) * 2 + 2];
            unsigned char V  = pp[(i * width + j * 2) * 2 + 3];
            
            // 밝기 향상 (Y 값 증가)
            Y0 = qBound(0, (int)(Y0 * 1.2), 255);  // 20% 밝게
            Y1 = qBound(0, (int)(Y1 * 1.2), 255);  // 20% 밝게
            
            // YUV -> RGB 변환
            // 첫 번째 픽셀
            int R0 = Y0 + 1.4075 * (V - 128);
            int G0 = Y0 - 0.3455 * (U - 128) - 0.7169 * (V - 128);
            int B0 = Y0 + 1.7790 * (U - 128);
            
            // 두 번째 픽셀
            int R1 = Y1 + 1.4075 * (V - 128);
            int G1 = Y1 - 0.3455 * (U - 128) - 0.7169 * (V - 128);
            int B1 = Y1 + 1.7790 * (U - 128);
            
            // 색상 보정: 붉은기 약간 감소
            R0 = qBound(0, (int)(R0 * 1.25), 255);   // 빨간색 25% 증가 (35%→25%)
            G0 = qBound(0, (int)(G0 * 0.85), 255);   // 녹색 15% 감소 (20%→15%)
            B0 = qBound(0, (int)(B0 * 0.85), 255);   // 파란색 15% 감소 (20%→15%)
            
            R1 = qBound(0, (int)(R1 * 1.25), 255);   // 빨간색 25% 증가 (35%→25%)
            G1 = qBound(0, (int)(G1 * 0.85), 255);   // 녹색 15% 감소 (20%→15%)
            B1 = qBound(0, (int)(B1 * 0.85), 255);   // 파란색 15% 감소 (20%→15%)
            
            // QImage에 픽셀 설정
            rgbImage.setPixel(j * 2, i, qRgb(R0, G0, B0));
            rgbImage.setPixel(j * 2 + 1, i, qRgb(R1, G1, B1));
        }
    }
}

QImage V4L2Camera::getCurrentFrame()
{
    QImage result;
    frameMutex.lock();
    result = currentFrame.copy();
    frameMutex.unlock();
    return result;
}

int V4L2Camera::xioctl(int fh, int request, void *arg)
{
    int r;

    do {
        r = ioctl(fh, request, arg);
    } while (r == -1 && errno == EINTR);

    return r;
} 