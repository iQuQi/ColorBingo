#ifndef BINGOWIDGET_H
#define BINGOWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QRandomGenerator>
#include <QSlider>
#include <QCheckBox>
#include <QEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QPair>
#include <QPixmap>
#include <QPushButton>
#include <QList>
#include <QColor>
#include <QFile>
#include <QSocketNotifier>
#include <linux/input.h>
#include "hardwareInterface/v4l2camera.h"
#include "hardwareInterface/webcambutton.h"

class BingoWidget : public QWidget {
    Q_OBJECT

public:
    explicit BingoWidget(QWidget *parent = nullptr, const QList<QColor> &initialColors = QList<QColor>());
    ~BingoWidget();
    bool eventFilter(QObject *obj, QEvent *event) override;
    bool isCameraCapturing() const { return isCapturing; }
    V4L2Camera* getCamera() const { return camera; }

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

signals:
    void backToMainRequested();


private slots:
    void updateCameraFrame();
    void handleCameraDisconnect();
    void onCircleSliderValueChanged(int value);
    void onCaptureButtonClicked();
    void clearXMark();
    void showSuccessMessage();
    void hideSuccessAndReset();
    void resetGame();
    void onRestartButtonClicked();
    void onTimerTick();
    void updateTimerDisplay();
    void updateTimerPosition();
    void startGameTimer();
    void stopGameTimer();
    void showFailMessage();
    void hideFailAndReset();
    void restartCamera();
    void onBackButtonClicked();
    void updateRgbValues();

private:
    // 빙고 관련 함수들
    void generateRandomColors();
    void updateCellStyle(int row, int col);
    QColor getCellColor(int row, int col);
    QString getCellColorName(int row, int col);
    int colorDistance(const QColor &c1, const QColor &c2);
    bool isColorBright(const QColor &color);
    void updateBingoScore();
    
    // 셀 선택 및 카메라 제어 함수
    void selectCell(int row, int col);
    void deselectCell();
    void startCamera();
    void stopCamera();
    
    // 원 내부 픽셀의 RGB 평균값 계산 함수
    void calculateAverageRGB(const QImage &image, int centerX, int centerY, int radius);
    
    // 카메라 관련 상태 변수
    bool isCapturing;           // 카메라 캡처 중인지 여부
    
    // 원 표시 관련 변수
    bool showCircle;            // 원 표시 여부
    int circleRadius;           // 원 반지름
    
    // RGB 평균값 관련 변수
    int avgRed;                 // 평균 R 값
    int avgGreen;               // 평균 G 값
    int avgBlue;                // 평균 B 값
    bool showRgbValues;         // RGB 값 표시 여부
    
    // 빙고 상태 변수
    QPair<int, int> selectedCell; // 현재 선택된 셀 좌표
    int bingoCount;             // 빙고 수
    
    // 레이아웃
    QHBoxLayout *mainLayout;
    QWidget *bingoArea;
    QWidget *cameraArea;
    QGridLayout *gridLayout;
    QVBoxLayout *cameraLayout;
//    QVBoxLayout *bingoLayout;
    
    // 빙고 관련 위젯
    QLabel *bingoCells[3][3];   // 빙고 셀 레이블
    QColor cellColors[3][3];    // 각 셀의 색상
    bool bingoStatus[3][3];     // 각 셀의 빙고 상태 (O 표시 여부)
    QLabel *bingoScoreLabel;    // 빙고 점수 표시 레이블
    
    // 카메라 관련 위젯
    QLabel *cameraView;
    V4L2Camera *camera;
    
    // 컨트롤 버튼
    QPushButton *startButton;
    QPushButton *captureButton;  // 캡처 및 중지 버튼으로 역할 변경
//    QPushButton *backButton;
    
    // 원 표시 관련 위젯
    QSlider *circleSlider;
    QCheckBox *circleCheckBox;
    QLabel *circleValueLabel;
    
    // RGB 값 표시 관련 위젯
    QCheckBox *rgbCheckBox;
    QLabel *cameraRgbValueLabel;

    // 타이머
    QTimer *fadeXTimer;         // X 표시 사라지는 타이머
    QTimer *successTimer;       // 성공 메시지 타이머
    QTimer *cameraRestartTimer; // 카메라 주기적 재시작 타이머

    // 성공 메시지 관련 멤버
    QLabel *successLabel;

    QPixmap xImage;

    // bearImage
    QPixmap bearImage;

    QPixmap createXImage(); // X 이미지 생성 함수
    QPixmap createBearImage(); // 곰돌이 이미지 생성 함수 추가

    // 색상 보정 관련 함수
    QImage adjustColorBalance(const QImage &image);
    QColor correctBluecast(const QColor &color);

    // Add this to the private section:
    QLabel *statusMessageLabel; // Label for displaying game status messages

    QPushButton* backButton; // Back 버튼
    QPushButton* restartButton; // Restart 버튼

    void updateBackButtonPosition();

    // 타이머 관련 변수
    QTimer* gameTimer;
    int remainingSeconds;
    QLabel* timerLabel;
    QLabel* failLabel;

    QWidget* sliderWidget;  // Circle slider container widget

    // 전달받은 색상 설정하는 메서드 추가
    void setCustomColors(const QList<QColor> &colors);

    // 기존 private 멤버 변수 영역에 추가
    QLabel *rValueLabel;
    QLabel *gValueLabel;
    QLabel *bValueLabel;

    // 대신 WebcamButton 객체 추가
    WebcamButton *webcamButton;

    // 누락된 함수 선언 추가
    void createCircleOverlay(int width, int height);

    // 멤버 변수들
    QPixmap overlayCircle;  // 원형 오버레이 픽스맵 추가

    // 선택된 셀의 RGB 값을 표시할 라벨 추가
    QLabel *cellRgbValueLabel;

    void updateCellRgbLabel(const QColor &color);
};

#endif // BINGOWIDGET_H
