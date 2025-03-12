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
#include "v4l2camera.h"

class BingoWidget : public QWidget {
    Q_OBJECT

public:
    explicit BingoWidget(QWidget *parent = nullptr);
    ~BingoWidget();
    void generateRandomColors();

private slots:
    void updateCameraFrame();
    void handleCameraDisconnect();
    void onCircleSliderValueChanged(int value);
    void onCircleCheckBoxToggled(bool checked);
    void onRgbCheckBoxToggled(bool checked);
    void onStartButtonClicked();
    void onStopButtonClicked();

private:
    // 원 내부 픽셀의 RGB 평균값 계산 함수
    void calculateAverageRGB(const QImage &image, int centerX, int centerY, int radius);
    
    // 레이아웃
    QHBoxLayout *mainLayout;
    QWidget *bingoArea;
    QWidget *cameraArea;
    QGridLayout *gridLayout;
    QVBoxLayout *cameraLayout;
    
    // 빙고 관련 멤버
    QLabel *bingoCells[3][3];
    
    // 카메라 관련 멤버
    QLabel *cameraView;
    V4L2Camera *camera;
    bool isCapturing;
    
    // 원 표시 관련 변수
    bool showCircle;
    int circleRadius;
    QSlider *circleSlider;
    QCheckBox *circleCheckBox;
    QLabel *circleValueLabel;
    
    // RGB 평균값 관련 변수
    int avgRed;
    int avgGreen;
    int avgBlue;
    bool showRgbValues;
    QCheckBox *rgbCheckBox;
    QLabel *rgbValueLabel;
    
    // 컨트롤 버튼
    QPushButton *startButton;
    QPushButton *stopButton;
};

#endif // BINGOWIDGET_H
