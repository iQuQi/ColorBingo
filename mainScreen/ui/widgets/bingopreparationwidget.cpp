#include "ui/widgets/bingopreparationwidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QRandomGenerator>
#include <QThread>
#include <algorithm>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>

BingoPreparationWidget::BingoPreparationWidget(QWidget *parent) :
    QWidget(parent),
    buttonPanel(nullptr),
    camera(nullptr),
    isCapturing(false),
    gameMode(GameMode::SINGLE)  // 기본값으로 SINGLE 모드 설정
{
    qDebug() << "DEBUG: BingoPreparationWidget constructor starting";

    // 네트워크
    network = P2PNetwork::getInstance();
    connect(network, &P2PNetwork::opponentDisconnected, this, &BingoPreparationWidget::onOpponentDisconnected);
    
    // 레이아웃 없이 절대 위치로 구성
    setLayout(nullptr);
    
    // 카메라 뷰 생성 (전체 화면)
    cameraView = new QLabel(this);
    cameraView->setAlignment(Qt::AlignCenter);
    cameraView->setFrameShape(QFrame::NoFrame);
    cameraView->setText("Camera connecting...");
    
    // 버튼 패널 생성 (떠 있는 형태)
    buttonPanel = new QWidget(this);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonPanel);
    buttonLayout->setSpacing(20);
    buttonLayout->setContentsMargins(20, 15, 20, 15);
    
    // 버튼 생성
    QPushButton *createBingoButton = new QPushButton("Create Bingo", buttonPanel);
    QPushButton *backButton = new QPushButton("Back", buttonPanel);
    
    // 버튼 크기 설정 - 더 크게 조정
    createBingoButton->setFixedSize(220, 70);  // 180,50 -> 220,70으로 키움
    backButton->setFixedSize(150, 70);         // 120,50 -> 150,70으로 키움
    
    // 파란색 스타일로 버튼 디자인 - 글씨 크기 키우고 굵게
    QString createButtonStyle = 
        "QPushButton {"
        "   font-size: 22px; font-weight: bold; "  // 18px -> 22px
        "   background-color: #4a86e8; color: white; "
        "   border-radius: 8px; "                  // 6px -> 8px
        "   padding: 5px; "                        // 패딩 추가
        "}"
        "QPushButton:hover {"
        "   background-color: #3a76d8; "
        "}"
        "QPushButton:pressed {"
        "   background-color: #2a66c8; "
        "}";
    
    QString backButtonStyle = 
        "QPushButton {"
        "   font-size: 22px; font-weight: bold; "  // 18px -> 22px
        "   background-color: #9e9e9e; color: white; "
        "   border-radius: 8px; "                  // 6px -> 8px
        "   padding: 5px; "                        // 패딩 추가
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
    connect(createBingoButton, &QPushButton::clicked, this, &BingoPreparationWidget::onCreateBingoClicked);
    connect(backButton, &QPushButton::clicked, this, &BingoPreparationWidget::onBackButtonClicked);
    
    // 카메라 초기화
    camera = new V4L2Camera(this);
    
    // 카메라 신호 연결
    connect(camera, &V4L2Camera::newFrameAvailable, this, &BingoPreparationWidget::updateCameraFrame);
    connect(camera, &V4L2Camera::deviceDisconnected, this, &BingoPreparationWidget::handleCameraDisconnect);
    
    // 초기 사이즈 설정
    resize(parent->size());
    
    // 카메라 열기는 필요하지만 시작은 showEvent에서 수행
    if (!camera->openCamera()) {
        QMessageBox::critical(this, "Error", "Failed to open camera device");
    } else {
        qDebug() << "Camera opened successfully";
    }
    
    qDebug() << "DEBUG: BingoPreparationWidget constructor completed";
}

void BingoPreparationWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    qDebug() << "DEBUG: BingoPreparationWidget showEvent triggered";
    
    // 위젯이 보여질 때 카메라 시작
    startCamera();
    
    // 카메라 뷰 업데이트를 위한 짧은 지연
    QTimer::singleShot(100, this, [this]() {
        if (cameraView) {
            updateCameraFrame();
        }
    });
}

void BingoPreparationWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    qDebug() << "DEBUG: BingoPreparationWidget hideEvent triggered";
    
    // 위젯이 숨겨질 때 카메라 중지
    stopCameraCapture();
}

void BingoPreparationWidget::startCamera() 
{
    qDebug() << "DEBUG: startCamera called";
    
    if (isCapturing) {
        qDebug() << "Camera is already capturing, no action needed";
        return;
    }
    
    if (!camera) {
        qDebug() << "ERROR: Camera object is null";
        return;
    }
    
    // 카메라가 닫혀있으면 다시 열기
    if (camera->getfd() == -1) {
        qDebug() << "Camera is closed, reopening";
        if (!camera->openCamera()) {
            qDebug() << "Failed to reopen camera";
            return;
        }
        // 카메라 열기 후 약간의 지연
        QThread::msleep(100);
    }
    
    // 카메라 캡처 시작
    if (camera->startCapturing()) {
        isCapturing = true;
        qDebug() << "Camera capture started successfully";
        
        // 카메라 뷰 초기화
        if (cameraView) {
            cameraView->clear();
        }
    } else {
        qDebug() << "Failed to start camera capture";
    }
}

void BingoPreparationWidget::stopCameraCapture() 
{
    qDebug() << "DEBUG: Stopping camera capture";
    
    if (camera) {
        camera->stopCapturing();
        isCapturing = false;
        qDebug() << "DEBUG: Camera capture stopped";
        
        // 확실한 리소스 해제를 위해 카메라도 닫기
        camera->closeCamera();
        qDebug() << "DEBUG: Camera closed completely";
        
        // 지연 추가
        QThread::msleep(100);
    }
}

void BingoPreparationWidget::updateCameraFrame() 
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

void BingoPreparationWidget::resizeEvent(QResizeEvent* event) 
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

void BingoPreparationWidget::handleCameraDisconnect()
{
    static bool isTransitioning = false;
    
    // 이미 전환 중이면 무시
    if (isTransitioning) {
        qDebug() << "DEBUG: Screen transition already in progress, ignoring";
        return;
    }
    
    isTransitioning = true;
    
    qDebug() << "DEBUG: Camera disconnected";
    
    if (cameraView) {
        cameraView->setText("Camera disconnected");
    }
    
    isCapturing = false;
    
    // 메인 화면으로 돌아가는 대화상자 표시
    QMessageBox::warning(this, "Camera Error", 
                         "Camera has been disconnected. Returning to main screen.");
    
    qDebug() << "DEBUG: Emitting backToMainRequested signal from handleCameraDisconnect";
    // 메인 화면으로 돌아가기
    emit backToMainRequested();
    
    // 전환 상태 재설정 (딜레이 후 리셋)
    QTimer::singleShot(500, []() { 
        isTransitioning = false; 
    });
}

void BingoPreparationWidget::onCreateBingoClicked() 
{
    qDebug() << "DEBUG: Create Bingo button clicked";
    
    // 색상 추출
    QList<QColor> capturedColors = captureColorsFromFrame();
    
    // 게임 모드에 따라 적절한 시그널 발생
    if (gameMode == GameMode::SINGLE) {
        qDebug() << "DEBUG: Emitting createBingoRequested for SINGLE mode";
        emit createBingoRequested(capturedColors);
    } else {
        qDebug() << "DEBUG: Emitting createMultiGameRequested for MULTI mode";
        emit createMultiGameRequested(capturedColors);
    }
}

void BingoPreparationWidget::onCreateMultiGameClicked()
{
    qDebug() << "DEBUG: Create Bingo button clicked";

    // 색상 추출
    QList<QColor> capturedColors = captureColorsFromFrame();

    // 색상 리스트와 함께 빙고 생성 요청 시그널 발생
    emit createMultiGameRequested(capturedColors);
}

void BingoPreparationWidget::onBackButtonClicked() 
{
    static bool isTransitioning = false;
    
    // 이미 전환 중이면 무시
    if (isTransitioning) {
        qDebug() << "DEBUG: Screen transition already in progress, ignoring";
        return;
    }
    
    isTransitioning = true;
    
    qDebug() << "DEBUG: Back button clicked";
    stopCameraCapture();
    
    qDebug() << "DEBUG: Emitting backToMainRequested signal from BingoPreparationWidget";
    emit backToMainRequested();
    
    // 전환 상태 재설정 (딜레이 후 리셋)
    QTimer::singleShot(500, []() { 
        isTransitioning = false; 
    });
}

// 소멸자
BingoPreparationWidget::~BingoPreparationWidget() 
{
    qDebug() << "DEBUG: BingoPreparationWidget destructor called";
    stopCameraCapture();
    
    // 카메라 객체 정리
    if (camera) {
        camera->closeCamera();
        // camera는 QObject 상속이므로 자동으로 제거됨
    }
    
    qDebug() << "DEBUG: BingoPreparationWidget destructor completed";
}

QList<QColor> BingoPreparationWidget::captureColorsFromFrame()
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

// setGameMode 메서드 구현
void BingoPreparationWidget::setGameMode(GameMode mode) 
{
    gameMode = mode;
    
    // buttonPanel 내의 Create Bingo 버튼 찾기
    if (buttonPanel) {
        QList<QPushButton*> buttons = buttonPanel->findChildren<QPushButton*>();
        for (QPushButton* button : buttons) {
            if (button->text() == "Create Bingo") {
                // 파란색 스타일로 버튼 디자인
                QString createButtonStyle = 
                    "QPushButton {"
                    "   font-size: 22px; font-weight: bold; "  // 18px -> 22px
                    "   background-color: #4a86e8; color: white; "
                    "   border-radius: 8px; "                  // 6px -> 8px
                    "   padding: 5px; "                        // 패딩 추가
                    "}"
                    "QPushButton:hover {"
                    "   background-color: #3a76d8; "
                    "}"
                    "QPushButton:pressed {"
                    "   background-color: #2a66c8; "
                    "}";
                
                // 파스텔 연초록 스타일 (멀티 게임용)
                QString createButtonGreenStyle = 
                    "QPushButton {"
                    "   font-size: 22px; font-weight: bold; "  // 업데이트된 사이즈
                    "   background-color: #8BC34A; color: white; "
                    "   border-radius: 8px; "                  // 업데이트된 radius
                    "   padding: 5px; "                        // 패딩 추가
                    "}"
                    "QPushButton:hover {"
                    "   background-color: #7CB342; "
                    "}"
                    "QPushButton:pressed {"
                    "   background-color: #689F38; "
                    "}";
                
                // 게임 모드에 따라 스타일 설정
                if (gameMode == GameMode::MULTI) {
                    qDebug() << "DEBUG: Setting green style for Create Bingo button (MULTI mode)";
                    button->setStyleSheet(createButtonGreenStyle);
                } else {
                    qDebug() << "DEBUG: Setting blue style for Create Bingo button (SINGLE mode)";
                    button->setStyleSheet(createButtonStyle);
                }
                break;
            }
        }
    }
} 

void BingoPreparationWidget::onOpponentDisconnected() {
    qDebug() << "DEBUG: Back to main from BingoPreparationWidget";
    emit backToMainRequested();
}
