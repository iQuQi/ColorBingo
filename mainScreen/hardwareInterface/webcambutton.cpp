#include <unistd.h>
#include <fcntl.h>
#include "hardwareInterface/webcambutton.h"

// ButtonReaderThread 구현
ButtonReaderThread::ButtonReaderThread(QObject *parent)
    : QThread(parent), stopRequested(false)
{
}

ButtonReaderThread::~ButtonReaderThread()
{
    stopReading();
    wait(); // 스레드가 종료될 때까지 대기
}

bool ButtonReaderThread::initialize(const QString &devicePath)
{
    this->devicePath = devicePath;
    return true;
}

void ButtonReaderThread::stopReading()
{
    QMutexLocker locker(&mutex);
    stopRequested = true;
}

void ButtonReaderThread::run()
{
    int fd = open(devicePath.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        qDebug() << "ButtonReaderThread: Cannot open device" << devicePath;
        return;
    }
    
    qDebug() << "ButtonReaderThread: Started reading from" << devicePath;
    
    struct input_event ev;
    int readBytes;
    
    // 이전 이벤트를 모두 비우기 위한 루프
    while ((readBytes = read(fd, &ev, sizeof(struct input_event))) == sizeof(struct input_event)) {
        // 이전 이벤트 비우기
    }
    
    while (!stopRequested) {
        // 비차단 모드로 읽기
        readBytes = read(fd, &ev, sizeof(struct input_event));
        
        if (readBytes == sizeof(struct input_event)) {
            // 버튼 누름 이벤트만 처리 (type=EV_KEY, value=1: 누름)
            if (ev.type == EV_KEY && ev.value == 1) {
                qDebug() << "ButtonReaderThread: Button pressed, key code:" << ev.code;
                emit buttonPressed(ev.code);
            }
        } else if (readBytes == -1 && errno != EAGAIN) {
            // EAGAIN은 데이터가 없음을 의미하므로 무시
            // 그 외 오류는 로그
            qDebug() << "ButtonReaderThread: Error reading:" << strerror(errno);
            break;
        }
        
        // CPU 사용량 감소를 위한 짧은 대기
        usleep(100000); // 100ms 대기
    }
    
    close(fd);
    qDebug() << "ButtonReaderThread: Stopped reading from" << devicePath;
}

// WebcamButton 구현
WebcamButton::WebcamButton(QObject *parent) : QObject(parent),
    readerThread(nullptr),
    initialized(false)
{
}

WebcamButton::~WebcamButton()
{
    if (readerThread) {
        readerThread->stopReading();
        readerThread->wait(); // 스레드가 종료될 때까지 대기
        delete readerThread;
    }
}

bool WebcamButton::initialize(const QString &devicePath)
{
    deviceFilePath = devicePath;
    
    // 이미 초기화된 경우 정리
    if (readerThread) {
        readerThread->stopReading();
        readerThread->wait();
        delete readerThread;
        readerThread = nullptr;
    }
    
    // 새 스레드 생성 및 시작
    readerThread = new ButtonReaderThread(this);
    if (!readerThread->initialize(devicePath)) {
        delete readerThread;
        readerThread = nullptr;
        initialized = false;
        return false;
    }
    
    // 버튼 신호 연결
    connect(readerThread, &ButtonReaderThread::buttonPressed, 
            this, &WebcamButton::handleButtonPressed);
    
    // 스레드 시작
    readerThread->start();
    
    initialized = true;
    qDebug() << "WebcamButton: Initialized successfully for" << devicePath;
    return true;
}

bool WebcamButton::isInitialized() const
{
    return initialized && readerThread && readerThread->isRunning();
}

void WebcamButton::handleButtonPressed(int keyCode)
{   
    // 카메라 캡처 버튼 확인
    if (keyCode == KEY_CAMERA || keyCode == 398 || keyCode == KEY_VOLUMEUP) {
        qDebug() << "WebcamButton: Camera capture button pressed";
        emit captureButtonPressed();
    }
} 