#ifndef WEBCAMBUTTON_H
#define WEBCAMBUTTON_H

#include <QObject>
#include <QFile>
#include <QSocketNotifier>
#include <QThread>
#include <QMutex>
#include <linux/input.h>
#include <QDebug>

// 버튼 이벤트 읽기를 담당할 스레드 클래스
class ButtonReaderThread : public QThread
{
    Q_OBJECT
    
public:
    ButtonReaderThread(QObject *parent = nullptr);
    ~ButtonReaderThread();
    
    // 스레드 시작 전 초기화
    bool initialize();
    void stopReading();

signals:
    // 버튼 누름 이벤트 발생 시 신호
    void buttonPressed(int keyCode);

protected:
    // 스레드 실행 함수
    void run() override;
    
private:
    // 웹캠 버튼 디바이스 파일 경로 찾기
    QString findWebcamButtonDevice();
    
    QString devicePath;
    bool stopRequested;
    QMutex mutex;
};

class WebcamButton : public QObject
{
    Q_OBJECT

public:
    explicit WebcamButton(QObject *parent = nullptr);
    ~WebcamButton();

    // 물리 버튼 초기화 함수
    bool initialize();
    
    // 입력 장치가 열려있는지 확인
    bool isInitialized() const;

signals:
    // 특정 목적 버튼이 눌렸을 때의 신호
    void captureButtonPressed();

private slots:
    // 버튼 누름 이벤트 처리
    void handleButtonPressed(int keyCode);

private:
    ButtonReaderThread *readerThread;  // 버튼 읽기 스레드
    QString deviceFilePath;            // 장치 파일 경로
    bool initialized;                 // 초기화 상태
};

#endif // WEBCAMBUTTON_H 