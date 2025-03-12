#include "bingowidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QSizePolicy>

BingoWidget::BingoWidget(QWidget *parent) : QWidget(parent),
    isCapturing(false),
    showCircle(true),
    circleRadius(15),
    avgRed(0),
    avgGreen(0),
    avgBlue(0),
    showRgbValues(true)
{
    // 메인 레이아웃 생성 (가로 분할)
    mainLayout = new QHBoxLayout(this);
    
    // 왼쪽 부분: 빙고 영역
    bingoArea = new QWidget(this);
    gridLayout = new QGridLayout(bingoArea);

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            bingoCells[row][col] = new QLabel(bingoArea);
            bingoCells[row][col]->setFixedSize(100, 100);
            bingoCells[row][col]->setAutoFillBackground(true);
            gridLayout->addWidget(bingoCells[row][col], row, col);
        }
    }

    bingoArea->setLayout(gridLayout);
    
    // 오른쪽 부분: 카메라 영역
    cameraArea = new QWidget(this);
    cameraLayout = new QVBoxLayout(cameraArea);
    
    // 카메라 뷰
    cameraView = new QLabel(cameraArea);
    cameraView->setMinimumSize(320, 240);
    cameraView->setFixedSize(320, 240);
    cameraView->setAlignment(Qt::AlignCenter);
    cameraView->setText("Camera connecting...");
    cameraView->setFrameShape(QFrame::Box);
    cameraView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    // 카메라 컨트롤
    QWidget *controlsWidget = new QWidget(cameraArea);
    QHBoxLayout *controlsLayout = new QHBoxLayout(controlsWidget);
    
    startButton = new QPushButton("Start", controlsWidget);
    stopButton = new QPushButton("Stop", controlsWidget);
    stopButton->setEnabled(false);
    
    controlsLayout->addWidget(startButton);
    controlsLayout->addWidget(stopButton);
    controlsWidget->setLayout(controlsLayout);
    
    // 원 설정 위젯
    QWidget *circleWidget = new QWidget(cameraArea);
    QVBoxLayout *circleLayout = new QVBoxLayout(circleWidget);
    
    // 체크박스
    circleCheckBox = new QCheckBox("Show Green Circle", circleWidget);
    circleCheckBox->setChecked(showCircle);
    
    // 슬라이더 및 값 표시
    QHBoxLayout *sliderLayout = new QHBoxLayout();
    QLabel *sliderLabel = new QLabel("Circle Size:", circleWidget);
    circleSlider = new QSlider(Qt::Horizontal, circleWidget);
    circleSlider->setMinimum(5);
    circleSlider->setMaximum(100);
    circleSlider->setValue(circleRadius);
    circleValueLabel = new QLabel(QString::number(circleRadius) + "%", circleWidget);
    circleValueLabel->setMinimumWidth(30);
    
    sliderLayout->addWidget(sliderLabel);
    sliderLayout->addWidget(circleSlider);
    sliderLayout->addWidget(circleValueLabel);
    
    // RGB 값 표시
    rgbCheckBox = new QCheckBox("Show RGB Average", circleWidget);
    rgbCheckBox->setChecked(showRgbValues);
    
    rgbValueLabel = new QLabel("R: 0  G: 0  B: 0", circleWidget);
    rgbValueLabel->setAlignment(Qt::AlignCenter);
    rgbValueLabel->setMinimumHeight(40);
    rgbValueLabel->setFrameShape(QFrame::Panel);
    rgbValueLabel->setFrameShadow(QFrame::Sunken);
    QFont font = rgbValueLabel->font();
    font.setBold(true);
    font.setPointSize(11);
    rgbValueLabel->setFont(font);
    
    // 원 설정 레이아웃 완성
    circleLayout->addWidget(circleCheckBox);
    circleLayout->addLayout(sliderLayout);
    circleLayout->addWidget(rgbCheckBox);
    circleLayout->addWidget(rgbValueLabel);
    circleWidget->setLayout(circleLayout);
    
    // 카메라 영역에 위젯 추가
    cameraLayout->addWidget(cameraView);
    cameraLayout->addWidget(controlsWidget);
    cameraLayout->addWidget(circleWidget);
    cameraLayout->addStretch();
    
    cameraArea->setLayout(cameraLayout);
    
    // 메인 레이아웃에 두 영역 추가
    mainLayout->addWidget(bingoArea);
    mainLayout->addWidget(cameraArea);
    
    // 분할 비율 고정 (왼쪽:오른쪽 = 1:1)
    mainLayout->setStretchFactor(bingoArea, 1);
    mainLayout->setStretchFactor(cameraArea, 1);
    
    // 각 영역이 고정된 너비를 유지하도록 설정
    bingoArea->setMinimumWidth(350);
    bingoArea->setMaximumWidth(350);
    cameraArea->setMinimumWidth(350);
    cameraArea->setMaximumWidth(350);
    
    setLayout(mainLayout);
    
    // 빙고 셀에 랜덤 색상 생성
    generateRandomColors();
    
    // 카메라 초기화
    camera = new V4L2Camera(this);
    
    // 카메라 신호 연결
    connect(camera, &V4L2Camera::newFrameAvailable, this, &BingoWidget::updateCameraFrame);
    connect(camera, &V4L2Camera::deviceDisconnected, this, &BingoWidget::handleCameraDisconnect);
    
    // 위젯 컨트롤 신호 연결
    connect(circleSlider, &QSlider::valueChanged, this, &BingoWidget::onCircleSliderValueChanged);
    connect(circleCheckBox, &QCheckBox::toggled, this, &BingoWidget::onCircleCheckBoxToggled);
    connect(rgbCheckBox, &QCheckBox::toggled, this, &BingoWidget::onRgbCheckBoxToggled);
    connect(startButton, &QPushButton::clicked, this, &BingoWidget::onStartButtonClicked);
    connect(stopButton, &QPushButton::clicked, this, &BingoWidget::onStopButtonClicked);
    
    // 카메라 열기 및 시작 시도
    if (camera->openCamera("/dev/video4")) {
        cameraView->setText("Click 'Start' button to start the camera");
    } else {
        cameraView->setText("Camera connection failed");
        startButton->setEnabled(false);
    }
}

BingoWidget::~BingoWidget() {
    if (camera) {
        camera->stopCapturing();
        camera->closeCamera();
    }
}

void BingoWidget::generateRandomColors() {
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            int r = QRandomGenerator::global()->bounded(256);
            int g = QRandomGenerator::global()->bounded(256);
            int b = QRandomGenerator::global()->bounded(256);

            QPalette palette = bingoCells[row][col]->palette();
            palette.setColor(QPalette::Window, QColor(r, g, b));
            bingoCells[row][col]->setPalette(palette);
        }
    }
}

void BingoWidget::updateCameraFrame() {
    // 카메라에서 새 프레임을 가져와 QLabel에 표시
    QImage frame = camera->getCurrentFrame();
    
    if (!frame.isNull()) {
        // 영상 크기에 맞게 스케일링하되 QLabel 크기는 유지
        QPixmap scaledPixmap = QPixmap::fromImage(frame).scaled(
            cameraView->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
            
        // 중앙에 녹색 원 그리기
        if (showCircle) {
            // 픽셀맵에 그리기 위한 QPainter 생성
            QPainter painter(&scaledPixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            
            // 원 중심 좌표 계산 (이미지 중앙)
            int centerX = scaledPixmap.width() / 2;
            int centerY = scaledPixmap.height() / 2;
            
            // 원의 반지름 계산 (이미지 너비의 percentage)
            int radius = (scaledPixmap.width() * circleRadius) / 100;
            
            // 녹색 반투명 펜 설정
            QPen pen(QColor(0, 255, 0, 180));
            pen.setWidth(2);
            painter.setPen(pen);
            
            // 원 그리기
            painter.drawEllipse(QPoint(centerX, centerY), radius, radius);
            
            // 원 영역 내부 RGB 평균 계산
            calculateAverageRGB(frame, frame.width()/2, frame.height()/2, 
                               (frame.width() * circleRadius) / 100);
                               
            // RGB 값 업데이트
            if (showRgbValues) {
                rgbValueLabel->setText(QString("R: %1  G: %2  B: %3").arg(avgRed).arg(avgGreen).arg(avgBlue));
                
                // 배경색 설정 (평균 RGB 값)
                QPalette palette = rgbValueLabel->palette();
                palette.setColor(QPalette::Window, QColor(avgRed, avgGreen, avgBlue));
                palette.setColor(QPalette::WindowText, (avgRed + avgGreen + avgBlue > 380) ? Qt::black : Qt::white);
                rgbValueLabel->setPalette(palette);
                rgbValueLabel->setAutoFillBackground(true);
            }
        }
        
        // QLabel에 이미지 표시 (크기 변경 없이)
        cameraView->setPixmap(scaledPixmap);
    }
}

void BingoWidget::calculateAverageRGB(const QImage &image, int centerX, int centerY, int radius) {
    long long sumR = 0, sumG = 0, sumB = 0;
    int pixelCount = 0;
    
    // 원 영역을 효율적으로 스캔하기 위해 원을 포함하는 사각형 영역만 처리
    int startX = qMax(centerX - radius, 0);
    int startY = qMax(centerY - radius, 0);
    int endX = qMin(centerX + radius, image.width() - 1);
    int endY = qMin(centerY + radius, image.height() - 1);
    
    // 사각형 내부의 각 픽셀에 대해
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            // 픽셀이 원 안에 있는지 확인 (피타고라스 정리 사용)
            int distX = x - centerX;
            int distY = y - centerY;
            int distSquared = distX * distX + distY * distY;
            
            if (distSquared <= radius * radius) {
                // 원 안에 있는 픽셀의 RGB 값 추출 및 합산
                QRgb pixel = image.pixel(x, y);
                sumR += qRed(pixel);
                sumG += qGreen(pixel);
                sumB += qBlue(pixel);
                pixelCount++;
            }
        }
    }
    
    // 픽셀이 없는 경우 예외 처리
    if (pixelCount > 0) {
        // 평균값 계산
        avgRed = sumR / pixelCount;
        avgGreen = sumG / pixelCount;
        avgBlue = sumB / pixelCount;
    } else {
        avgRed = avgGreen = avgBlue = 0;
    }
}

void BingoWidget::handleCameraDisconnect() {
    cameraView->setText("Camera disconnected");
    qDebug() << "Camera disconnected";
    
    // 버튼 상태 업데이트
    startButton->setEnabled(false);
    stopButton->setEnabled(false);
    isCapturing = false;
}

void BingoWidget::onCircleSliderValueChanged(int value) {
    // 원 크기 값 업데이트
    circleRadius = value;
    circleValueLabel->setText(QString::number(value) + "%");
}

void BingoWidget::onCircleCheckBoxToggled(bool checked) {
    // 원 표시 여부 설정
    showCircle = checked;
    circleSlider->setEnabled(checked);
    
    // RGB 값 표시 관련 UI 요소 활성화/비활성화
    if (checked) {
        rgbCheckBox->setEnabled(true);
        rgbValueLabel->setEnabled(showRgbValues);
    } else {
        rgbCheckBox->setEnabled(false);
        rgbValueLabel->setEnabled(false);
    }
}

void BingoWidget::onRgbCheckBoxToggled(bool checked) {
    showRgbValues = checked;
    rgbValueLabel->setEnabled(checked);
}

void BingoWidget::onStartButtonClicked() {
    // 카메라 캡처 시작
    if (!isCapturing) {
        if (!camera->startCapturing()) {
            QMessageBox::critical(this, "Error", "Failed to start capture");
            return;
        }
        
        // 버튼 상태 업데이트
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
        
        // 플래그 설정
        isCapturing = true;
    }
}

void BingoWidget::onStopButtonClicked() {
    // 카메라 캡처 중지
    if (isCapturing) {
        // 카메라 캡처 중지
        camera->stopCapturing();
        
        // 버튼 상태 업데이트
        startButton->setEnabled(true);
        stopButton->setEnabled(false);
        
        // 플래그 설정
        isCapturing = false;
        
        // 마지막 프레임 유지
        cameraView->setText("Click 'Start' button to start the camera");
    }
}
