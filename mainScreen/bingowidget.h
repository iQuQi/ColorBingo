#ifndef BINGOWIDGET_H
#define BINGOWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QRandomGenerator>
#include <QSlider>
#include <QCheckBox>
#include <QEvent>
#include <QMouseEvent>
#include "v4l2camera.h"
#include <QTimer>
#include <QThread>
#include <QPair>

class BingoWidget : public QWidget {
    Q_OBJECT

public:
    explicit BingoWidget(QWidget *parent = nullptr);
    ~BingoWidget();
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void updateCameraFrame();
    void handleCameraDisconnect();
    void onCircleSliderValueChanged(int value);
    void onCircleCheckBoxToggled(bool checked);
    void onRgbCheckBoxToggled(bool checked);
    void onStartButtonClicked();
    void onStopButtonClicked();
    void restartCamera();
    void onCaptureButtonClicked();
    void clearXMark();

private:
    // 빙고 관련 함수들
    void generateRandomColors();
    void updateCellStyle(int row, int col);
    QColor getCellColor(int row, int col);
    QString getCellColorName(int row, int col);
    int colorDistance(const QColor &c1, const QColor &c2);
    bool isColorBright(const QColor &color);
    void updateBingoScore();
    
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
    QPushButton *stopButton;
    QPushButton *captureButton;  // 캡처 버튼 추가
    
    // 원 표시 관련 위젯
    QSlider *circleSlider;
    QCheckBox *circleCheckBox;
    QLabel *circleValueLabel;
    
    // RGB 값 표시 관련 위젯
    QCheckBox *rgbCheckBox;
    QLabel *rgbValueLabel;

    // 타이머
    QTimer *cameraRestartTimer;  // 카메라 재시작 타이머
    QTimer *fadeXTimer;         // X 표시 사라지는 타이머
};

#endif // BINGOWIDGET_H