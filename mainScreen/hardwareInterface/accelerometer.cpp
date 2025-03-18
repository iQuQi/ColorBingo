#include <unistd.h>
#include <fcntl.h>
#include <QTextStream>
#include <QRegularExpression>
#include <QDir>
#include "hardwareInterface/accelerometer.h"

// AccelerometerReaderThread 구현
AccelerometerReaderThread::AccelerometerReaderThread(QObject *parent)
    : QThread(parent), stopRequested(false)
{
    // 초기화
    currentData.x = 0;
    currentData.y = 0;
    currentData.z = 0;
}

AccelerometerReaderThread::~AccelerometerReaderThread()
{
    stopReading();
    wait(); // 스레드가 종료될 때까지 대기
}

bool AccelerometerReaderThread::initialize()
{
    qDebug() << "AccelerometerReaderThread: Attempting to find accelerometer device automatically";
    this->devicePath = findAccelerometerDevice();
    
    // 여전히 경로가 없으면 기본값 설정
    if (this->devicePath.isEmpty()) {
        qDebug() << "AccelerometerReaderThread: Automatic detection failed, using default path";
        this->devicePath = "/dev/input/event0"; // 가속도 센서는 주로 event0에 연결됨
    }

    qDebug() << "AccelerometerReaderThread: Device path is" << devicePath;
    return true;
}

void AccelerometerReaderThread::stopReading()
{
    QMutexLocker locker(&mutex);
    stopRequested = true;
}

void AccelerometerReaderThread::run()
{
    // 파일 경로 로컬 변수에 저장
    QString localDevicePath = devicePath;
    
    // 장치 파일 열기 시도
    int fd = open(localDevicePath.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
    
    // 열기 실패 시 신호 발생 후 종료
    if (fd == -1) {
        qDebug() << "AccelerometerReaderThread: Cannot open device" << localDevicePath << "- Error:" << strerror(errno);
        emit deviceDisconnected();
        return;
    }
    
    qDebug() << "AccelerometerReaderThread: Started reading from" << localDevicePath;
    
    struct input_event ev;
    int readBytes;
    
    // 이전 이벤트를 모두 비우기 위한 루프
    while ((readBytes = read(fd, &ev, sizeof(struct input_event))) == sizeof(struct input_event)) {
        // 이전 이벤트 비우기
    }
    
    AccelerometerData data = {0, 0, 0}; // 현재 데이터 초기화
    bool dataUpdated = false;
    
    while (!stopRequested) {
        // 비차단 모드로 읽기
        readBytes = read(fd, &ev, sizeof(struct input_event));
        
        if (readBytes == sizeof(struct input_event)) {
            // ABS 이벤트 처리 (가속도 센서 데이터)
            if (ev.type == EV_ABS) {
                switch (ev.code) {
                    case ABS_X:
                        data.x = ev.value;
                        dataUpdated = true;
                        break;
                    case ABS_Y:
                        data.y = ev.value;
                        dataUpdated = true;
                        break;
                    case ABS_Z:
                        data.z = ev.value;
                        dataUpdated = true;
                        break;
                }
                
                // 모든 축 데이터가 업데이트되었을 때 신호 발생
                if (dataUpdated && (ev.code == ABS_Z || ev.code == ABS_Y || ev.code == ABS_X)) {
                    QMutexLocker locker(&mutex);
                    currentData = data;
                    locker.unlock();
                    emit accelerometerDataChanged(data);
                    dataUpdated = false;
                    // qDebug() << "AccelerometerReaderThread: Data changed - X:" << data.x << " Y:" << data.y << " Z:" << data.z;
                }
            }
        } else if (readBytes == -1 && errno != EAGAIN) {
            // EAGAIN은 데이터가 없음을 의미하므로 무시
            // 그 외 오류는 로그 및 장치 연결 해제 신호 발생
            qDebug() << "AccelerometerReaderThread: Error reading:" << strerror(errno);
            emit deviceDisconnected();
            break;
        }
        
        // CPU 사용량 감소를 위한 짧은 대기
        usleep(10000); // 10ms 대기 (가속도 센서는 보통 더 높은 빈도로 데이터를 제공)
    }
    
    close(fd);
    qDebug() << "AccelerometerReaderThread: Stopped reading from" << localDevicePath;
}

QString AccelerometerReaderThread::findAccelerometerDevice() {
    QFile file("/proc/bus/input/devices");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "AccelerometerReaderThread: Cannot open /proc/bus/input/devices";
        // 파일을 열 수 없으면 기본값 반환
        return "/dev/input/event0";
    }
    
    qDebug() << "AccelerometerReaderThread: Successfully opened /proc/bus/input/devices file";
    
    // 파일 전체 내용을 로그로 출력 (디버깅용)
    QTextStream logStream(&file);
    QString entireFileContent = logStream.readAll();
    qDebug() << "==== /proc/bus/input/devices full content ====";
    qDebug().noquote() << entireFileContent;
    qDebug() << "==== /proc/bus/input/devices content end ====";
    
    // 파일 포인터를 다시 처음으로 되돌림
    file.seek(0);
    
    // 가속도 센서 장치 정보
    bool foundAccelerometer = false;
    QString currentEventDevice;
    
    QTextStream in(&file);
    QString line;
    while (!(line = in.readLine()).isNull()) {
        // 가속도 센서 장치 이름 확인 (Accelerometer 문자열이 포함된 이름)
        if (line.startsWith("N:") && line.contains("Accelerometer", Qt::CaseInsensitive)) {
            qDebug() << "AccelerometerReaderThread: Found accelerometer device:" << line;
            foundAccelerometer = true;
            continue;
        }
        
        // 핸들러(이벤트 번호) 확인
        if (foundAccelerometer && line.startsWith("H:") && line.contains("event")) {
            qDebug() << "AccelerometerReaderThread: Found accelerometer event handler:" << line;
            
            // 이벤트 번호 추출
            QRegularExpression regex("event(\\d+)");
            QRegularExpressionMatch match = regex.match(line);
            if (match.hasMatch()) {
                currentEventDevice = "/dev/input/event" + match.captured(1);
                qDebug() << "AccelerometerReaderThread: Extracted accelerometer event device:" << currentEventDevice;
                
                // 장치를 찾았으므로 파일을 닫고 장치 경로 반환
                file.close();
                return currentEventDevice;
            }
        }
        
        // 새 장치 시작 표시
        if (line.startsWith("I:") && foundAccelerometer) {
            // 이전 가속도 센서 장치에서 이벤트를 찾지 못했으므로 초기화
            foundAccelerometer = false;
        }
    }
    
    file.close();
    
    // 가속도 센서 장치를 찾지 못했거나 이벤트를 추출하지 못한 경우 기본값 반환
    qDebug() << "AccelerometerReaderThread: Accelerometer device not found, using default event0";
    return "/dev/input/event0";
}

// Accelerometer 구현
Accelerometer::Accelerometer(QObject *parent) : QObject(parent),
    readerThread(nullptr),
    initialized(false)
{
    // AccelerometerData 타입을 Qt 메타 타입 시스템에 등록
    qRegisterMetaType<AccelerometerData>("AccelerometerData");
    
    // 초기화
    currentData.x = 0;
    currentData.y = 0;
    currentData.z = 0;
}

Accelerometer::~Accelerometer()
{
    if (readerThread) {
        readerThread->stopReading();
        readerThread->wait(); // 스레드가 종료될 때까지 대기
        delete readerThread;
    }
}

bool Accelerometer::initialize()
{
    // 이미 초기화된 경우 정리
    if (readerThread) {
        readerThread->stopReading();
        readerThread->wait();
        delete readerThread;
        readerThread = nullptr;
    }
    
    // 새 스레드 생성 및 시작
    readerThread = new AccelerometerReaderThread(this);
    if (!readerThread->initialize()) {
        delete readerThread;
        readerThread = nullptr;
        initialized = false;
        return false;
    }
    
    // 시그널 연결
    connect(readerThread, &AccelerometerReaderThread::accelerometerDataChanged, 
            this, &Accelerometer::handleAccelerometerDataChanged);
    connect(readerThread, &AccelerometerReaderThread::deviceDisconnected,
            this, &Accelerometer::handleDeviceDisconnected);
    
    // 스레드 시작
    readerThread->start();
    
    initialized = true;
    qDebug() << "Accelerometer: Initialized successfully";
    return true;
}

bool Accelerometer::isInitialized() const
{
    return initialized && readerThread && readerThread->isRunning();
}

AccelerometerData Accelerometer::getCurrentData() const
{
    return currentData;
}

void Accelerometer::handleAccelerometerDataChanged(const AccelerometerData &data)
{
    // 현재 데이터 업데이트
    currentData = data;
    
    // 신호 전달
    emit accelerometerDataChanged(data);
}

void Accelerometer::handleDeviceDisconnected()
{
    qDebug() << "Accelerometer: Device disconnected";
    initialized = false;
    
    // 신호 전달
    emit deviceDisconnected();
} 