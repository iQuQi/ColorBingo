#include "colorcapturewidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QRandomGenerator>
#include <QThread>
#include <algorithm>
#include <QTimer>

ColorCaptureWidget::ColorCaptureWidget(QWidget *parent) :
    QWidget(parent),
    buttonPanel(nullptr),
    camera(nullptr),
    isCapturing(false)
{
    qDebug() << "DEBUG: ColorCaptureWidget constructor starting";
    
    // 레이아웃 없이 절대 위치로 구성
    setLayout(nullptr);
    
    // 카메라 뷰 생성 (전체 화면)
    cameraView = new QLabel(this);
    cameraView->setAlignment(Qt::AlignCenter);
    cameraView->setFrameShape(QFrame::NoFrame);
    cameraView->setText("");
    
    // 버튼 패널 생성 (떠 있는 형태)
    buttonPanel = new QWidget(this);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonPanel);
    buttonLayout->setSpacing(15);
    buttonLayout->setContentsMargins(15, 10, 15, 10);
    
    // 버튼 생성
    QPushButton *createBingoButton = new QPushButton("Create Bingo", buttonPanel);
    QPushButton *backButton = new QPushButton("Back", buttonPanel);
    
    // 버튼 크기 설정
    createBingoButton->setFixedSize(180, 50);
    backButton->setFixedSize(120, 50);
    
    // 파란색 스타일로 버튼 디자인
    QString createButtonStyle = 
        "QPushButton {"
        "   font-size: 18px; font-weight: bold; "
        "   background-color: #4a86e8; color: white; "
        "   border-radius: 6px; "
        "}"
        "QPushButton:hover {"
        "   background-color: #3a76d8; "
        "}"
        "QPushButton:pressed {"
        "   background-color: #2a66c8; "
        "}";
    
    QString backButtonStyle = 
        "QPushButton {"
        "   font-size: 18px; font-weight: bold; "
        "   background-color: #9e9e9e; color: white; "
        "   border-radius: 6px; "
        "}"
        "QPushButton:hover {"
        "   background-color: #8e8e8e; "
        "}"
        "QPushButton:pressed {"
        "   background-color: #7e7e7e; "
        "}";
    
    createBingoButton->setStyleSheet(createButtonStyle);
    backButton->setStyleSheet(backButtonStyle);
    
    buttonLayout->addWidget(backButton);
    buttonLayout->addWidget(createBingoButton);
    
    // 버튼 패널 배경 설정
    buttonPanel->setStyleSheet("background-color: rgba(30, 30, 30, 180); border-radius: 10px;");
    
    // 시그널 연결
    connect(createBingoButton, &QPushButton::clicked, this, &ColorCaptureWidget::onCreateBingoClicked);
    connect(backButton, &QPushButton::clicked, this, &ColorCaptureWidget::onBackButtonClicked);
    
    // 카메라 초기화
    camera = new V4L2Camera(this);
    
    // 카메라 신호 연결
    connect(camera, &V4L2Camera::newFrameAvailable, this, &ColorCaptureWidget::updateCameraFrame);
    connect(camera, &V4L2Camera::deviceDisconnected, this, &ColorCaptureWidget::handleCameraDisconnect);
    
    // 초기 사이즈 설정
    resize(parent->size());
    
    // 카메라 열기
    if (!camera->openCamera("/dev/video4")) {
        cameraView->setText("Camera connection failed");
        qDebug() << "Failed to open camera";
    } else {
        qDebug() << "Camera opened successfully";
        
        // 카메라 시작
        qDebug() << "startCamera called";
        startCamera();
    }
    
    qDebug() << "DEBUG: ColorCaptureWidget constructor completed";
    
    // 디버그용 타이머 - 위젯 가시성 확인
    QTimer::singleShot(500, this, [this]() {
        qDebug() << "DEBUG: Camera view visible:" << cameraView->isVisible();
        qDebug() << "DEBUG: Button panel visible:" << buttonPanel->isVisible();
    });
}

void ColorCaptureWidget::startCamera() 
{
    if (!isCapturing && camera) {
        if (camera->startCapturing()) {
            isCapturing = true;
            qDebug() << "Camera capture started successfully";
        } else {
            qDebug() << "Failed to start camera capture";
        }
    }
}

void ColorCaptureWidget::stopCameraCapture() 
{
    qDebug() << "DEBUG: Stopping camera capture";
    if (isCapturing && camera) {
        camera->stopCapturing();
        isCapturing = false;
        qDebug() << "DEBUG: Camera capture stopped";
        
        // 확실한 리소스 해제를 위해 카메라도 닫기
        camera->closeCamera();
        qDebug() << "DEBUG: Camera closed completely";
        
        // 지연 추가
        QThread::msleep(1000);
    }
}

void ColorCaptureWidget::updateCameraFrame() 
{
    if (!camera || !cameraView) return;
    
    QImage frame = camera->getCurrentFrame();
    if (!frame.isNull()) {
        // 카메라 프레임 크기를 라벨 크기에 맞게 조정 (비율 유지하지 않고 꽉 채움)
        QPixmap pixmap = QPixmap::fromImage(frame).scaled(
            cameraView->width(),
            cameraView->height(),
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation);
        
        cameraView->setPixmap(pixmap);
    }
}

void ColorCaptureWidget::resizeEvent(QResizeEvent* event) 
{
    QWidget::resizeEvent(event);
    
    // 카메라 뷰가 전체 공간을 채우도록 설정
    if (cameraView) {
        cameraView->setGeometry(0, 0, width(), height());
    }
    
    // 버튼 패널이 하단 중앙에 위치하도록 설정
    if (buttonPanel) {
        buttonPanel->adjustSize();
        int x = (width() - buttonPanel->width()) / 2;
        int y = height() - buttonPanel->height() - 40; // 하단에서 40px 여백
        buttonPanel->move(x, y);
        buttonPanel->raise(); // 항상 맨 위에 표시
    }
    
    // 카메라 프레임 업데이트
    updateCameraFrame();
}

void ColorCaptureWidget::handleCameraDisconnect()
{
    qDebug() << "DEBUG: Camera disconnected";
    
    if (cameraView) {
        cameraView->setText("Camera disconnected");
    }
    
    isCapturing = false;
    
    // 메인 화면으로 돌아가는 대화상자 표시
    QMessageBox::warning(this, "Camera Error", 
                         "Camera has been disconnected. Returning to main screen.");
    
    // 메인 화면으로 돌아가기
    emit backToMainRequested();
}

void ColorCaptureWidget::onCreateBingoClicked() 
{
    qDebug() << "DEBUG: Create Bingo button clicked";
    
    // 색상 추출
    QList<QColor> capturedColors = captureColorsFromFrame();
    
    // 색상 리스트와 함께 빙고 생성 요청 시그널 발생
    emit createBingoRequested(capturedColors);
}

void ColorCaptureWidget::onBackButtonClicked() 
{
    qDebug() << "DEBUG: Back button clicked";
    stopCameraCapture();
    emit backToMainRequested();
}

// 소멸자
ColorCaptureWidget::~ColorCaptureWidget() 
{
    qDebug() << "DEBUG: ColorCaptureWidget destructor called";
    stopCameraCapture();
    
    // 카메라 객체 정리
    if (camera) {
        camera->closeCamera();
        // camera는 QObject 상속이므로 자동으로 제거됨
    }
    
    qDebug() << "DEBUG: ColorCaptureWidget destructor completed";
}

QList<QColor> ColorCaptureWidget::captureColorsFromFrame()
{
    QList<QColor> capturedColors;
    
    // 카메라 프레임에서 색상 추출
    QImage frame = camera->getCurrentFrame();
    if (!frame.isNull()) {
        int width = frame.width();
        int height = frame.height();
        
        // 정확히 9개의 색상을 카메라에서 추출
        struct SamplePoint {
            double xRatio, yRatio;
            QString name;
        };
        
        // 샘플링 지점 정의 - 9개 위치
        SamplePoint samplePoints[] = {
            {0.2, 0.2, "좌측 상단"},
            {0.5, 0.2, "중앙 상단"},
            {0.8, 0.2, "우측 상단"},
            {0.2, 0.5, "좌측 중앙"},
            {0.5, 0.5, "중앙"},
            {0.8, 0.5, "우측 중앙"},
            {0.2, 0.8, "좌측 하단"},
            {0.5, 0.8, "중앙 하단"},
            {0.8, 0.8, "우측 하단"}
        };
        
        // 각 샘플링 지점에서 색상 추출
        for (const auto &point : samplePoints) {
            int centerX = width * point.xRatio;
            int centerY = height * point.yRatio;
            
            // 해당 지점 주변 25x25 영역의 평균 색상 계산
            long totalR = 0, totalG = 0, totalB = 0;
            int count = 0;
            
            for (int sy = centerY - 12; sy <= centerY + 12; sy++) {
                for (int sx = centerX - 12; sx <= centerX + 12; sx++) {
                    if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                        QColor pixelColor = frame.pixelColor(sx, sy);
                        totalR += pixelColor.red();
                        totalG += pixelColor.green();
                        totalB += pixelColor.blue();
                        count++;
                    }
                }
            }
            
            if (count > 0) {
                QColor avgColor(totalR / count, totalG / count, totalB / count);
                capturedColors.append(avgColor);
                qDebug() << "Captured color from" << point.name << ":" 
                        << avgColor.red() << avgColor.green() << avgColor.blue();
            } else {
                // 추출 실패한 경우만 랜덤 색상 추가
                QColor randomColor(
                    QRandomGenerator::global()->bounded(256),
                    QRandomGenerator::global()->bounded(256),
                    QRandomGenerator::global()->bounded(256)
                );
                capturedColors.append(randomColor);
                qDebug() << "Failed to capture from" << point.name << ", added random color:" 
                        << randomColor.red() << randomColor.green() << randomColor.blue();
            }
        }
    } else {
        // 프레임 자체가 없는 경우 9개의 랜덤 색상 생성
        qDebug() << "No valid camera frame, generating 9 random colors";
        for (int i = 0; i < 9; i++) {
            QColor randomColor(
                QRandomGenerator::global()->bounded(256),
                QRandomGenerator::global()->bounded(256),
                QRandomGenerator::global()->bounded(256)
            );
            capturedColors.append(randomColor);
            qDebug() << "Added random color " << i+1 << ":" 
                    << randomColor.red() << randomColor.green() << randomColor.blue();
        }
    }
    
    return capturedColors;  // 정확히 9개의 색상 반환
} 