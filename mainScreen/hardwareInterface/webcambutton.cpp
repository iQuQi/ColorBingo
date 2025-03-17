#include <unistd.h>
#include <fcntl.h>
#include <QTextStream>
#include <QRegularExpression>
#include <QDir>
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

bool ButtonReaderThread::initialize()
{
    if(devicePath.isNull() || devicePath.isEmpty())
    {
        qDebug() << "ButtonReaderThread: Attempting to find webcam button device automatically";
        this->devicePath = findWebcamButtonDevice();
        
        // 여전히 경로가 없으면 기본값 설정
        if (this->devicePath.isEmpty()) {
            qDebug() << "ButtonReaderThread: Automatic detection failed, using default path";
            this->devicePath = "/dev/input/event1"; // 대부분의 시스템에서 키보드 이벤트
        }
    }

    qDebug() << "ButtonReaderThread: Device path is" << devicePath;

    return true;
}

void ButtonReaderThread::stopReading()
{
    QMutexLocker locker(&mutex);
    stopRequested = true;
}

void ButtonReaderThread::run()
{
    // 파일 경로 로컬 변수에 저장
    QString localDevicePath = devicePath;
    
    // 장치 파일 열기 시도
    int fd = open(localDevicePath.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
    
    // 열기 실패 시 다른 장치 시도
    if (fd == -1) {
        qDebug() << "ButtonReaderThread: Cannot open device" << localDevicePath << "- Error:" << strerror(errno);
        qDebug() << "ButtonReaderThread: Failed to open any input device, aborting";
        return;
    }
    
    qDebug() << "ButtonReaderThread: Started reading from" << localDevicePath;
    
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
    qDebug() << "ButtonReaderThread: Stopped reading from" << localDevicePath;
}

QString ButtonReaderThread::findWebcamButtonDevice() {
    QFile file("/proc/bus/input/devices");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "ButtonReaderThread: Cannot open /proc/bus/input/devices";
        // 파일을 열 수 없으면 기본값 반환
        return "/dev/input/event1";  // 웹캠 버튼이 주로 event1에 있음
    }
    
    qDebug() << "ButtonReaderThread: Successfully opened /proc/bus/input/devices file";
    
    // 파일 전체 내용을 로그로 출력 (디버깅용)
    QTextStream logStream(&file);
    QString entireFileContent = logStream.readAll();
    qDebug() << "==== /proc/bus/input/devices 전체 내용 ====";
    qDebug().noquote() << entireFileContent;
    qDebug() << "==== /proc/bus/input/devices 내용 끝 ====";
    
    // 파일 포인터를 다시 처음으로 되돌림
    file.seek(0);
    
    // 웹캠 장치 정보
    QString webcamName = "USB2.0 PC CAMERA";  // 웹캠 장치 이름
    bool foundWebcam = false;
    QString currentEventDevice;
    
    QTextStream in(&file);
    QString line;
    while (!(line = in.readLine()).isNull()) {
        // 웹캠 장치 이름 확인
        if (line.startsWith("N:") && line.contains(webcamName, Qt::CaseInsensitive)) {
            qDebug() << "ButtonReaderThread: Found webcam device:" << line;
            foundWebcam = true;
            continue;
        }
        
        // 버튼 물리 경로 확인 (추가 확인)
        if (foundWebcam && line.startsWith("P:") && line.contains("button", Qt::CaseInsensitive)) {
            qDebug() << "ButtonReaderThread: Confirmed webcam button:" << line;
            // foundWebcam은 이미 true이므로 추가 설정 불필요
            continue;
        }
        
        // 핸들러(이벤트 번호) 확인
        if (foundWebcam && line.startsWith("H:") && line.contains("event")) {
            qDebug() << "ButtonReaderThread: Found webcam event handler:" << line;
            
            // 이벤트 번호 추출
            QRegularExpression regex("event(\\d+)");
            QRegularExpressionMatch match = regex.match(line);
            if (match.hasMatch()) {
                currentEventDevice = "/dev/input/event" + match.captured(1);
                qDebug() << "ButtonReaderThread: Extracted webcam event device:" << currentEventDevice;
                
                // 장치를 찾았으므로 파일을 닫고 장치 경로 반환
                file.close();
                return currentEventDevice;
            }
        }
        
        // 새 장치 시작 표시
        if (line.startsWith("I:") && foundWebcam) {
            // 이전 웹캠 장치에서 이벤트를 찾지 못했으므로 초기화
            foundWebcam = false;
        }
    }
    
    file.close();
    
    // 웹캠 장치를 찾지 못했거나 이벤트를 추출하지 못한 경우 기본값 반환
    qDebug() << "ButtonReaderThread: Webcam device not found, using default event1";
    return "/dev/input/event1";  // 웹캠 버튼이 주로 event1에 있음
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

bool WebcamButton::initialize()
{
    // 이미 초기화된 경우 정리
    if (readerThread) {
        readerThread->stopReading();
        readerThread->wait();
        delete readerThread;
        readerThread = nullptr;
    }
    
    // 새 스레드 생성 및 시작
    readerThread = new ButtonReaderThread(this);
    if (!readerThread->initialize()) {
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
    qDebug() << "WebcamButton: Initialized successfully";
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
        qDebug() << "WebcamButton: button pressed"<< keyCode;
        emit captureButtonPressed();
    }
    else {
        // 지원하지 않는 키 코드는 무시
        qDebug() << "WebcamButton: Ignoring unsupported key code:" << keyCode;
    }
}