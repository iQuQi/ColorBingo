#include <QDebug>
#include <fcntl.h>
#include <QThread>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "hardwareInterface/v4l2camera.h"
#include <QElapsedTimer>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

V4L2Camera::V4L2Camera(QObject *parent) :
    QObject(parent),
    fd(-1),
    buffers(NULL),
    n_buffers(0),
    isCapturing(false),
    stopThread(false),
    devicePath("/dev/video4")  // 디바이스 경로를 기본값으로 초기화
{
}

V4L2Camera::~V4L2Camera()
{
    closeCamera();
}

// 기본 디바이스 경로를 사용하는 openCamera 구현
bool V4L2Camera::openCamera()
{
    return openCamera(devicePath);
}

bool V4L2Camera::openCamera(const QString &deviceName)
{
    struct stat st;

    if (fd != -1) {
        qDebug() << "Camera already open, closing first.";
        closeCamera();
        return true;
    }
    
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
    fmt.fmt.pix.width = 800;
    fmt.fmt.pix.height = 600;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    qDebug() << "fmt.fmt.pix.width:" << fmt.fmt.pix.width;
    qDebug() << "fmt.fmt.pix.height:" << fmt.fmt.pix.height;

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
        // Set frame rate to 45fps (time interval = 1second/fps)
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = 45;
        
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
        if (!openCamera()) {
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
    
    // 프레임 처리 빈도 제한을 위한 변수
    QElapsedTimer frameTimer;
    frameTimer.start();
    const int MIN_FRAME_INTERVAL_MS = 33; // 약 30 FPS로 제한 (1000/30 = 33.3ms)

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
        
        // 이전 프레임 처리 후 최소 시간이 경과했는지 확인
        if (frameTimer.elapsed() < MIN_FRAME_INTERVAL_MS) {
            // 너무 빠른 프레임 처리는 CPU 부하를 줄이기 위해 건너뜀
            // 다음 이벤트가 발생할 때까지 대기하면서 스레드 슬립
            QThread::msleep(5); // 아주 짧게 쉬어 CPU 사용 줄임
            continue;
        }

        if (readFrame()) {
            emit newFrameAvailable();
            frameTimer.restart(); // 타이머 재시작
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
    
    // 더 빠른 방식으로 변환을 위한 사전 계산된 테이블 사용
    // 이 테이블들은 정적으로 선언하면 매번 재계산하지 않아도 됨
    static int table_Y[256];
    static int table_Cb_blue[256];
    static int table_Cb_green[256];
    static int table_Cr_green[256];
    static int table_Cr_red[256];
    static bool tables_initialized = false;
    
    if (!tables_initialized) {
        // 테이블 초기화 (한번만 수행)
        for (int i = 0; i < 256; i++) {
            table_Y[i] = qBound(0, (int)(i * 1.2), 255); // 밝기 향상 20%
            
            int Cb_blue = (int)(1.7790 * (i - 128));
            int Cb_green = (int)(0.3455 * (i - 128));
            int Cr_green = (int)(0.7169 * (i - 128));
            int Cr_red = (int)(1.4075 * (i - 128));
            
            table_Cb_blue[i] = Cb_blue;
            table_Cb_green[i] = -Cb_green;
            table_Cr_green[i] = -Cr_green;
            table_Cr_red[i] = Cr_red;
        }
        tables_initialized = true;
    }
    
    #pragma omp parallel for  // OpenMP 병렬화 (시스템 지원 시)
    for (int i = 0; i < height; i++) {
        // 각 열에 대해 루프 최적화를 위해 포인터 사용
        unsigned char *pY0, *pU, *pY1, *pV;
        int rowOffset = i * width * 2;  // YUV422의 각 픽셀은 2바이트 사용
        
        // 한 행씩 처리
        for (int j = 0; j < width / 2; j++) {
            int pixelOffset = rowOffset + j * 4;  // 2개 픽셀마다 4바이트 (Y0 U Y1 V)
            
            pY0 = pp + pixelOffset;
            pU = pY0 + 1;
            pY1 = pY0 + 2;
            pV = pY0 + 3;
            
            // 밝기 값 가져오기 (이미 테이블에서 밝기 보정 적용됨)
            int Y0 = table_Y[*pY0];
            int Y1 = table_Y[*pY1];
            int U = *pU;
            int V = *pV;
            
            // RGB 값 계산
            int R0 = Y0 + table_Cr_red[V];
            int G0 = Y0 + table_Cb_green[U] + table_Cr_green[V];
            int B0 = Y0 + table_Cb_blue[U];
            
            int R1 = Y1 + table_Cr_red[V];
            int G1 = Y1 + table_Cb_green[U] + table_Cr_green[V];
            int B1 = Y1 + table_Cb_blue[U];
            
            // 색상 보정: 붉은기 약간 증가
            R0 = qBound(0, (int)(R0), 255);   // 빨간색 25% 증가
            G0 = qBound(0, (int)(G0), 255);   // 녹색 15% 감소
            B0 = qBound(0, (int)(B0), 255);   // 파란색 15% 감소
            
            R1 = qBound(0, (int)(R1), 255);   // 빨간색 25% 증가
            G1 = qBound(0, (int)(G1), 255);   // 녹색 15% 감소
            B1 = qBound(0, (int)(B1), 255);   // 파란색 15% 감소
            
            // 픽셀 설정
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
