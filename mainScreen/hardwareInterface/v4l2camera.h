#ifndef V4L2CAMERA_H
#define V4L2CAMERA_H

#include <QObject>
#include <QImage>
#include <QMutex>
#include <linux/videodev2.h>
#include <pthread.h>

class V4L2Camera : public QObject
{
    Q_OBJECT

public:
    explicit V4L2Camera(QObject *parent = 0);
    ~V4L2Camera();

    bool openCamera(const QString &deviceName);
    bool openCamera();
    void closeCamera();
    bool startCapturing();
    void stopCapturing();
    QImage getCurrentFrame();
    bool isCameraCapturing() const { return isCapturing; }
    int getfd() const { return fd; }

signals:
    void newFrameAvailable();
    void deviceDisconnected();

private:
    void initDevice();
    void uninitDevice();
    void initMMAP();
    bool readFrame();
    void processImage(const void *p, int size);
    int xioctl(int fh, int request, void *arg);
    void yuv422ToRgb888(const void *yuv, QImage &rgbImage);

    static void *captureThreadFunc(void *arg);
    void captureThreadLoop();

    struct Buffer {
        void *start;
        size_t length;
    };

    int fd;
    struct v4l2_format fmt;
    struct Buffer *buffers;
    unsigned int n_buffers;
    bool isCapturing;
    QImage currentFrame;
    QMutex frameMutex;
    pthread_t captureThread;
    bool stopThread;
    QString devicePath;
};

#endif // V4L2CAMERA_H 
