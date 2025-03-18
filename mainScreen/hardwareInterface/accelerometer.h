#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <QObject>
#include <QFile>
#include <QSocketNotifier>
#include <QThread>
#include <QMutex>
#include <linux/input.h>
#include <QDebug>

// 가속도 데이터를 저장하는 구조체
struct AccelerometerData {
    int x;
    int y;
    int z;
};

// Qt 메타 타입 시스템에 AccelerometerData 등록
Q_DECLARE_METATYPE(AccelerometerData)

// 가속도 센서 이벤트 읽기를 담당할 스레드 클래스
class AccelerometerReaderThread : public QThread
{
    Q_OBJECT
    
public:
    AccelerometerReaderThread(QObject *parent = nullptr);
    ~AccelerometerReaderThread();
    
    // 스레드 시작 전 초기화
    bool initialize();
    void stopReading();

signals:
    // 가속도 값 변경 이벤트 발생 시 신호
    void accelerometerDataChanged(const AccelerometerData &data);
    // 장치 연결 해제 신호
    void deviceDisconnected();

protected:
    // 스레드 실행 함수
    void run() override;
    
private:
    // 가속도 센서 디바이스 파일 경로 찾기
    QString findAccelerometerDevice();
    
    QString devicePath;
    bool stopRequested;
    QMutex mutex;
    
    // 현재 가속도 데이터
    AccelerometerData currentData;
};

class Accelerometer : public QObject
{
    Q_OBJECT

public:
    explicit Accelerometer(QObject *parent = nullptr);
    ~Accelerometer();

    // 가속도 센서 초기화 함수
    bool initialize();
    
    // 입력 장치가 열려있는지 확인
    bool isInitialized() const;
    
    // 현재 가속도 데이터 반환
    AccelerometerData getCurrentData() const;

signals:
    // 가속도 값이 변경되었을 때의 신호
    void accelerometerDataChanged(const AccelerometerData &data);
    // 장치 연결 해제 신호
    void deviceDisconnected();

private slots:
    // 가속도 데이터 변경 이벤트 처리
    void handleAccelerometerDataChanged(const AccelerometerData &data);
    // 장치 연결 해제 처리
    void handleDeviceDisconnected();

private:
    AccelerometerReaderThread *readerThread;  // 센서 읽기 스레드
    bool initialized;                         // 초기화 상태
    AccelerometerData currentData;            // 최신 가속도 데이터
};

#endif // ACCELEROMETER_H 