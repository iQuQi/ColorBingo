#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "hardwareInterface/SoundManager.h"
#include "ui/widgets/colorcapturewidget.h"
#include "background.h"  // 내장된 배경 리소스 포함
#include <QDebug>
#include <QThread>
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QIcon>
#include <QPolygon>
#include <QPen>
#include <QGraphicsDropShadowEffect>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    colorCaptureWidget(nullptr),
    bingoWidget(nullptr),
    matchingWidget(nullptr),
    multiGameWidget(nullptr),
    volumeLevel(2), // Default volume level is 2 (medium)
    backgroundImage() // 배경 이미지 초기화 추가
{
    // Set default window size
    resize(800, 600);
    
    // Initialize stacked widget
    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);
    
    // Initialize main menu screen
    mainMenu = new QWidget();
    
    // Initialize main screen (add content to mainMenu)
    setupMainScreen();
    
    // Add main menu to stacked widget
    stackedWidget->addWidget(mainMenu);
    stackedWidget->setCurrentWidget(mainMenu);

    // Initial volume setting (medium - 0.75)
    SoundManager::getInstance()->setBackgroundVolume(0.75f);
    SoundManager::getInstance()->setEffectVolume(0.75f);
    
    // Start background music
    SoundManager::getInstance()->playBackgroundMusic();
    
    // Create and setup volume button
    volumeButton = new QPushButton(this);
    volumeButton->setFixedSize(60, 60);
    volumeButton->setStyleSheet(
        "QPushButton { "
        "   background-color: rgba(255, 255, 255, 180); "
        "   border-radius: 10px; "
        "   border: 3px solid #444444; "
        "} "
        "QPushButton:hover { "
        "   background-color: rgba(255, 255, 255, 220); "
        "   border: 3px solid #4a86e8; "
        "} "
        "QPushButton:pressed { "
        "   background-color: rgba(220, 220, 220, 220); "
        "} "
        "QPushButton:focus { "
        "   border: 3px solid #444444; "
        "   outline: none; "
        "}"
    );
    volumeButton->setCursor(Qt::PointingHandCursor);
    volumeButton->setFocusPolicy(Qt::NoFocus);
    connect(volumeButton, &QPushButton::clicked, this, &MainWindow::onVolumeButtonClicked);
    
    // Initial volume button update
    updateVolumeButton();
    // Set initial volume button position
    updateVolumeButtonPosition();
    
    // 디버깅 - 작업 디렉토리 확인
    qDebug() << "Working directory:" << QDir::currentPath();
    
    // Qt 리소스 시스템에서 내장된 배경 이미지 로드
    backgroundImage = BackgroundResource::getBackgroundImage();
    qDebug() << "Loaded embedded background image:" << !backgroundImage.isNull();
    if (!backgroundImage.isNull()) {
        qDebug() << "Background image size:" << backgroundImage.width() << "x" << backgroundImage.height();
    } else {
        qDebug() << "Failed to load background image from resources. Check resources.qrc file.";
    }
    
    qDebug() << "MainWindow creation completed, mainMenu size:" << mainMenu->size();
    qDebug() << "stackedWidget size:" << stackedWidget->size();
    qDebug() << "MainWindow size:" << size();
}

// Pixel style bear image creation function
QPixmap MainWindow::createBearImage() {
    QPixmap bearImage(80, 80);
    bearImage.fill(Qt::transparent);
    QPainter painter(&bearImage);
    
    // 안티앨리어싱 비활성화 (픽셀 느낌을 위해)
    painter.setRenderHint(QPainter::Antialiasing, false);
    
    // 갈색 곰돌이 색상
    QColor bearColor(165, 113, 78);
    QColor darkBearColor(120, 80, 60);
    
    // 중앙 정렬을 위한 오프셋
    int offsetX = 5;
    int offsetY = 5;
    
    // 기본 얼굴 사각형
    painter.setPen(Qt::NoPen);
    painter.setBrush(bearColor);
    painter.drawRect(15 + offsetX, 20 + offsetY, 40, 40);
    
    // 얼굴 둥글게 만들기 - 픽셀 추가
    painter.drawRect(11 + offsetX, 25 + offsetY, 4, 30);  // 왼쪽
    painter.drawRect(55 + offsetX, 25 + offsetY, 4, 30);  // 오른쪽
    painter.drawRect(20 + offsetX, 16 + offsetY, 30, 4);  // 위
    painter.drawRect(20 + offsetX, 60 + offsetY, 30, 4);  // 아래
    
    // 추가 픽셀로 더 둥글게 표현
    painter.drawRect(15 + offsetX, 20 + offsetY, 5, 5);   // 좌상단 보강
    painter.drawRect(50 + offsetX, 20 + offsetY, 5, 5);   // 우상단 보강
    painter.drawRect(15 + offsetX, 55 + offsetY, 5, 5);   // 좌하단 보강
    painter.drawRect(50 + offsetX, 55 + offsetY, 5, 5);   // 우하단 보강
    
    // 모서리 픽셀 추가
    painter.drawRect(12 + offsetX, 21 + offsetY, 3, 4);   // 좌상단 모서리
    painter.drawRect(55 + offsetX, 21 + offsetY, 3, 4);   // 우상단 모서리
    painter.drawRect(12 + offsetX, 55 + offsetY, 3, 4);   // 좌하단 모서리
    painter.drawRect(55 + offsetX, 55 + offsetY, 3, 4);   // 우하단 모서리
    
    // 귀 위치 및 크기 조정 (가로 길이 축소)
    // 왼쪽 귀 - 가로 길이 축소 (13→10)
    painter.drawRect(16 + offsetX, 6 + offsetY, 10, 16);  // 기본 왼쪽 귀 (가로 축소)
    painter.drawRect(11 + offsetX, 10 + offsetY, 5, 12);  // 왼쪽 귀 왼쪽 보강
    painter.drawRect(26 + offsetX, 10 + offsetY, 5, 12);  // 왼쪽 귀 오른쪽 보강 (좌표 조정)
    
    // 오른쪽 귀 - 가로 길이 축소 (13→10)
    painter.drawRect(44 + offsetX, 6 + offsetY, 10, 16);  // 기본 오른쪽 귀 (가로 축소)
    painter.drawRect(39 + offsetX, 10 + offsetY, 5, 12);  // 오른쪽 귀 왼쪽 보강 (좌표 조정)
    painter.drawRect(54 + offsetX, 10 + offsetY, 5, 12);  // 오른쪽 귀 오른쪽 보강
    
    // 귀 안쪽 (더 어두운 색) - 가로 길이 축소 (7→6)
    painter.setBrush(darkBearColor);
    painter.drawRect(19 + offsetX, 9 + offsetY, 6, 10);   // 왼쪽 귀 안쪽 (가로 축소)
    painter.drawRect(45 + offsetX, 9 + offsetY, 6, 10);   // 오른쪽 귀 안쪽 (가로 축소)
    
    // 눈 (간격 넓히기)
    painter.setBrush(Qt::black);
    painter.drawRect(22 + offsetX, 35 + offsetY, 6, 6);   // 왼쪽 눈 (좌표 조정 - 더 왼쪽으로)
    painter.drawRect(42 + offsetX, 35 + offsetY, 6, 6);   // 오른쪽 눈 (좌표 조정 - 더 오른쪽으로)
    
    // 코 (위치 위로 올리고 크기 축소)
    painter.drawRect(32 + offsetX, 42 + offsetY, 6, 4);   // 코 (위치 위로, 크기 축소 8x5→6x4)
    
    return bearImage;
}

void MainWindow::setupMainScreen()
{
    // Setup main screen widget
    QVBoxLayout *mainLayout = new QVBoxLayout(mainMenu);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Set background style - 배경 제거하고 투명하게 설정
    mainMenu->setStyleSheet("QWidget { background-color: transparent; }");
    
    // Create central container for content
    centerWidget = new QWidget(mainMenu);
    centerWidget->setStyleSheet("QWidget { background-color: #ffffff; border-radius: 10px; }");
    centerWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    // 그림자 효과 추가
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 0);
    centerWidget->setGraphicsEffect(shadow);
    
    QVBoxLayout *centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setSpacing(20);
    centerLayout->setContentsMargins(30, 30, 30, 30);
    
    // Create horizontal layout for title area
    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->setAlignment(Qt::AlignVCenter); // Align all elements to vertical center
    
    // Create bear image
    QPixmap bearPixmap = createBearImage();
    
    // Left bear image label
    QLabel *leftBearLabel = new QLabel(centerWidget);
    leftBearLabel->setPixmap(bearPixmap);
    leftBearLabel->setFixedSize(bearPixmap.size());
    
    // Title label
    QLabel *titleLabel = new QLabel("Color Bingo", centerWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    // Remove bottom margin and keep only font size
    titleLabel->setStyleSheet("font-size: 48px; font-weight: bold; color: #333;");
    
    // Right bear image label
    QLabel *rightBearLabel = new QLabel(centerWidget);
    rightBearLabel->setPixmap(bearPixmap);
    rightBearLabel->setFixedSize(bearPixmap.size());
    
    // Add widgets to layout (all set to vertical center alignment)
    titleLayout->addWidget(leftBearLabel, 0, Qt::AlignVCenter);
    titleLayout->addWidget(titleLabel, 1, Qt::AlignVCenter); // 1 is stretch factor, takes more space
    titleLayout->addWidget(rightBearLabel, 0, Qt::AlignVCenter);
    
    // Add top and bottom margin to title layout (keep overall spacing)
    titleLayout->setContentsMargins(0, 0, 0, 20);
    
    // Add title layout to main layout
    centerLayout->addLayout(titleLayout);
    
    // Set common button size
    const int BUTTON_WIDTH = 280;
    const int BUTTON_HEIGHT = 70;
    
    // Button common style
    QString blueButtonStyle = 
        "QPushButton {"
        "   font-size: 24px; font-weight: bold; padding: 15px 30px;"
        "   background-color: #4a86e8; color: white; border-radius: 8px;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: #3a76d8;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #2a66c8;"
        "}";
    
    // Add pastel light green style
    QString greenButtonStyle = 
        "QPushButton {"
        "   font-size: 24px; font-weight: bold; padding: 15px 30px;"
        "   background-color: #8BC34A; color: white; border-radius: 8px;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: #7CB342;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #689F38;"
        "}";
    
    QString redButtonStyle = 
        "QPushButton {"
        "   font-size: 24px; font-weight: bold; padding: 15px 30px;"
        "   background-color: #e74c3c; color: white; border-radius: 8px;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: #d62c1a;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #c0392b;"
        "}";
    
    // Single Game button (previously Start Bingo button)
    QPushButton *singleGameButton = new QPushButton("Single Game", centerWidget);
    singleGameButton->setStyleSheet(blueButtonStyle);
    singleGameButton->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    singleGameButton->setCursor(Qt::PointingHandCursor);
    centerLayout->addWidget(singleGameButton, 0, Qt::AlignCenter);
    
    // Multi Game button (newly added)
    QPushButton *multiGameButton = new QPushButton("Multi Game", centerWidget);
    multiGameButton->setStyleSheet(greenButtonStyle);
    multiGameButton->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    multiGameButton->setCursor(Qt::PointingHandCursor);
    centerLayout->addWidget(multiGameButton, 0, Qt::AlignCenter);
    
    // Exit button
    QPushButton *exitButton = new QPushButton("Exit", centerWidget);
    exitButton->setStyleSheet(redButtonStyle);
    exitButton->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    exitButton->setCursor(Qt::PointingHandCursor);
    centerLayout->addWidget(exitButton, 0, Qt::AlignCenter);
    
    // Adjust container size
    centerWidget->adjustSize();
    
    // Add stretch and central container to main layout
    mainLayout->addStretch(1);
    mainLayout->addWidget(centerWidget, 0, Qt::AlignCenter);
    mainLayout->addStretch(1);
    
    // Connect signals
    connect(singleGameButton, &QPushButton::clicked, this, &MainWindow::showBingoScreen);
    //connect(multiGameButton, &QPushButton::clicked, this, &MainWindow::showMultiGameScreen);
    connect(multiGameButton, &QPushButton::clicked, this, [=]() {
        this->showMatchingScreen();
        matchingWidget->startMatching();
    });
    connect(exitButton, &QPushButton::clicked, this, &QMainWindow::close);
    
    qDebug() << "setupMainScreen completed, centerWidget size:" << centerWidget->size();
}

// Single game screen display (previously onStartBingoClicked)
void MainWindow::showBingoScreen()
{
    qDebug() << "DEBUG: Single Game button clicked";
    
    // 현재 멀티게임 위젯이 활성화되어 있다면 카메라 리소스 해제
//    if (multiGameWidget && stackedWidget->currentWidget() == multiGameWidget) {
//        qDebug() << "DEBUG: Ensuring MultiGameWidget camera is stopped";
        
//        // 카메라 리소스 완전 해제를 위한 명시적 처리 (임시 대기)
//        QThread::msleep(500);
//    }
    
    // 기존 bingoWidget이 있고 카메라가 실행 중이라면 리소스 해제
//    if (bingoWidget && bingoWidget->isCameraCapturing()) {
    if ((bingoWidget && bingoWidget->isCameraCapturing()) || (multiGameWidget && multiGameWidget->isCameraCapturing())) {
        qDebug() << "DEBUG: Ensuring BingoWidget camera is stopped";
        V4L2Camera* camera = bingoWidget->getCamera();
        if (camera) {
            camera->stopCapturing();
            camera->closeCamera();
            qDebug() << "DEBUG: BingoWidget camera explicitly stopped";
        }
        
        // Wait time for complete camera resource release
        QThread::msleep(500);
    }
    
    if (!colorCaptureWidget) {
        qDebug() << "DEBUG: Creating new ColorCaptureWidget";
        colorCaptureWidget = new ColorCaptureWidget(this);
        // Add to stacked widget
        stackedWidget->addWidget(colorCaptureWidget);
    }
    
    // Set to single game mode
    colorCaptureWidget->setGameMode(GameMode::SINGLE);
    qDebug() << "DEBUG: ColorCaptureWidget set to SINGLE mode";
    
    // Disconnect previous connections and set up new ones
    qDebug() << "DEBUG: Reconnecting signals for Single Game mode";
    disconnect(colorCaptureWidget, nullptr, this, nullptr); // Disconnect all existing connections
    connect(colorCaptureWidget, &ColorCaptureWidget::createBingoRequested, 
            this, &MainWindow::onCreateBingoRequested);
    connect(colorCaptureWidget, &ColorCaptureWidget::backToMainRequested, 
            this, &MainWindow::showMainMenu);
    
    // Change current widget to color capture widget
    stackedWidget->setCurrentWidget(colorCaptureWidget);
    
    // Add short delay to allow resources to fully initialize
    QTimer::singleShot(500, this, [this]() {
        qDebug() << "DEBUG: ColorCaptureWidget now displayed";
    });
}


// 매칭 화면 표시 (멀티게임 버튼 클릭 시, 멀티게임 화면 표시 전)
void MainWindow::showMatchingScreen() {
    if (!matchingWidget) {
         qDebug() << "DEBUG: Creating new MatchingWidget";
         matchingWidget = new MatchingWidget(this);
         stackedWidget->addWidget(matchingWidget);
         connect(matchingWidget, &MatchingWidget::switchToBingoScreen, this, &MainWindow::showMultiGameScreen);
    }
    qDebug() << "DEBUG: Multi Game button clicked";

    stackedWidget->setCurrentWidget(matchingWidget);
}


// 멀티게임 화면 표시 (새로 추가)
// Multi game screen display (newly added)
void MainWindow::showMultiGameScreen()
{
    qDebug() << "DEBUG: Show Multi Game Screen";

    // 현재 멀티게임 위젯이 활성화되어 있다면 카메라 리소스 해제
//    if (multiGameWidget && stackedWidget->currentWidget() == multiGameWidget) {
//        qDebug() << "DEBUG: Ensuring MultiGameWidget camera is stopped";

//        // 카메라 리소스 완전 해제를 위한 명시적 처리 (임시 대기)
//        QThread::msleep(500);
//    }

    // 기존 multiGameWidget이 있고 카메라가 실행 중이라면 리소스 해제
//    if (multiGameWidget && multiGameWidget->isCameraCapturing()) {
    if ((bingoWidget && bingoWidget->isCameraCapturing()) || (multiGameWidget && multiGameWidget->isCameraCapturing())) {
        qDebug() << "DEBUG: Ensuring BingoWidget camera is stopped";
        V4L2Camera* camera = multiGameWidget->getCamera();
        if (camera) {
            camera->stopCapturing();
            camera->closeCamera();
            qDebug() << "DEBUG: BingoWidget camera explicitly stopped";
        }

        // Wait time for complete camera resource release
        QThread::msleep(500);
    }

    if (!colorCaptureWidget) {
        qDebug() << "DEBUG: Creating new ColorCaptureWidget";
        colorCaptureWidget = new ColorCaptureWidget(this);
        // Add to stacked widget
        stackedWidget->addWidget(colorCaptureWidget);
    }

    // Set to multi game mode
    colorCaptureWidget->setGameMode(GameMode::MULTI);
    qDebug() << "DEBUG: ColorCaptureWidget set to MULTI mode";
    
    // Disconnect previous connections and set up new ones
    qDebug() << "DEBUG: Reconnecting signals for Multi Game mode";
    disconnect(colorCaptureWidget, nullptr, this, nullptr); // Disconnect all existing connections
    connect(colorCaptureWidget, &ColorCaptureWidget::createMultiGameRequested,
            this, &MainWindow::onCreateMultiGameRequested);
    connect(colorCaptureWidget, &ColorCaptureWidget::backToMainRequested,
            this, &MainWindow::showMainMenu);

    // Change current widget to color capture widget
    stackedWidget->setCurrentWidget(colorCaptureWidget);

    // Add short delay to allow resources to fully initialize
    QTimer::singleShot(500, this, [this]() {
        qDebug() << "DEBUG: ColorCaptureWidget now displayed";
    });
}

void MainWindow::onCreateBingoRequested(const QList<QColor> &colors)
{
    qDebug() << "DEBUG: Create Bingo requested with" << colors.size() << "colors";
    
    // Release camera resources - fully close
    if (colorCaptureWidget) {
        qDebug() << "DEBUG: Stopping camera before creating BingoWidget";
        colorCaptureWidget->stopCameraCapture();
        
        // Wait time for camera resource release
        qDebug() << "DEBUG: Waiting for camera resources to be fully released";
        QThread::msleep(1500); // Wait 1.5 seconds
    }
    
    // Safely clean up existing bingoWidget if present
    if (bingoWidget) {
        qDebug() << "DEBUG: Cleaning up previous BingoWidget";
        disconnect(bingoWidget); // Disconnect signals
        stackedWidget->removeWidget(bingoWidget);
        delete bingoWidget;
        bingoWidget = nullptr;
    }

    if (multiGameWidget) {
        qDebug() << "DEBUG: Cleaning up previous MultiGameWidget";
        disconnect(multiGameWidget); // Disconnect signals
        stackedWidget->removeWidget(multiGameWidget);
        delete multiGameWidget;
        multiGameWidget = nullptr;
    }
    
    // Create BingoWidget
    qDebug() << "DEBUG: Creating BingoWidget";
    bingoWidget = new BingoWidget(this, colors);
    
    // Connect signals
    connect(bingoWidget, &BingoWidget::backToMainRequested,
            this, &MainWindow::showMainMenu, Qt::QueuedConnection);
    



    // Add to stacked widget and set as current widget

    stackedWidget->addWidget(bingoWidget);
    stackedWidget->setCurrentWidget(bingoWidget);
    qDebug() << "DEBUG: BingoWidget now displayed";
}

void MainWindow::onCreateMultiGameRequested(const QList<QColor> &colors)
{
    qDebug() << "DEBUG: Create Multi Game requested with" << colors.size() << "colors";

    // Release camera resources - fully close
    if (colorCaptureWidget) {
        qDebug() << "DEBUG: Stopping camera before creating MultiGameWidget";
        colorCaptureWidget->stopCameraCapture();

        // Wait time for camera resource release
        qDebug() << "DEBUG: Waiting for camera resources to be fully released";
        QThread::msleep(1500); // Wait 1.5 seconds
    }

    // Safely clean up existing bingoWidget if present
    if (bingoWidget) {
        qDebug() << "DEBUG: Cleaning up previous BingoWidget";
        disconnect(bingoWidget); // Disconnect signals
        stackedWidget->removeWidget(bingoWidget);
        delete bingoWidget;
        bingoWidget = nullptr;
    }

    if (multiGameWidget) {
        qDebug() << "DEBUG: Cleaning up previous MultiGameWidget";
        disconnect(multiGameWidget); // Disconnect signals
        stackedWidget->removeWidget(multiGameWidget);
        delete multiGameWidget;
        multiGameWidget = nullptr;
    }

    // Create MultiGameWidget
    qDebug() << "DEBUG: Creating MultiGameWidget";
    multiGameWidget = new MultiGameWidget(this, colors);

    // Connect signals
    connect(multiGameWidget, &MultiGameWidget::backToMainRequested,
            this, &MainWindow::showMainMenu, Qt::QueuedConnection);

    // Add to stacked widget and set as current widget
    stackedWidget->addWidget(multiGameWidget);
    stackedWidget->setCurrentWidget(multiGameWidget);
    qDebug() << "DEBUG: MultiGameWidget now displayed";
}

// Display main menu (previously onBingoBackRequested)
void MainWindow::showMainMenu()
{
    qDebug() << "DEBUG: Back to main menu requested - SAFE HANDLING";
    
    // Ensure widget camera resources are released
    if (colorCaptureWidget && stackedWidget->currentWidget() == colorCaptureWidget) {
        qDebug() << "DEBUG: Stopping ColorCaptureWidget camera before returning to main menu";
        colorCaptureWidget->stopCameraCapture();
        QThread::msleep(500);
    }
    
    if (bingoWidget && bingoWidget->isCameraCapturing()) {
        qDebug() << "DEBUG: Stopping BingoWidget camera before returning to main menu";
        V4L2Camera* camera = bingoWidget->getCamera();
        if (camera) {
            camera->stopCapturing();
            camera->closeCamera();
        }
        QThread::msleep(500);
    }
    
    if (multiGameWidget && multiGameWidget->isCameraCapturing()) {
        qDebug() << "DEBUG: Stopping MultiGameWidget camera before returning to main menu";
        V4L2Camera* camera = multiGameWidget->getCamera();
        if (camera) {
            camera->stopCapturing();
            camera->closeCamera();
        }
        QThread::msleep(500);
    }
    
    // Delay object deletion until event loop completes
    QTimer::singleShot(0, this, [this]() {
        qDebug() << "DEBUG: Executing delayed widget cleanup";
        
        // Switch to main menu
        stackedWidget->setCurrentWidget(mainMenu);
        qDebug() << "DEBUG: Main menu now displayed";
    });
//=======
/*
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(stackedWidget);
    setLayout(mainLayout);
//    ui->setupUi(this);


    // connect(startButton, &QPushButton::clicked, this, &MainWindow::showBingoScreen);
    connect(startButton, &QPushButton::clicked, this, [=]() {
        this->showMatchingScreen();
        matchingWidget->startMatching();

    });
    connect(exitButton, &QPushButton::clicked, this, &QWidget::close);
    //connect(bingoWidget, &BingoWidget::backToMainRequested, this, &MainWindow::showMainMenu);
    connect(bingoWidget, &BingoWidget::backToMainRequested, this, [=]() {
        this->showMainMenu();
        matchingWidget->p2p->disconnectFromPeer();
    });
    connect(matchingWidget, &MatchingWidget::switchToBingoScreen, this, &MainWindow::showBingoScreen);
*/
}



bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        updateCenterWidgetPosition();
        
        // 음량 버튼 위치 업데이트 - 분리된 함수 호출
        updateVolumeButtonPosition();
    }
    return QMainWindow::event(event);
}

void MainWindow::updateCenterWidgetPosition()
{
    if (mainMenu && centerWidget) {
        int x = (mainMenu->width() - centerWidget->width()) / 2;
        int y = (mainMenu->height() - centerWidget->height()) / 2;
        centerWidget->move(x, y);
    }
}

// Volume control button click event handler
void MainWindow::onVolumeButtonClicked()
{
    // Save previous level
    int oldLevel = volumeLevel;
    
    // Cycle volume level: 2(medium) -> 3(high) -> 1(low) -> 2(medium)
    volumeLevel = (volumeLevel % 3) + 1;
    
    // Set SoundManager volume
    float volume = 0.0f;
    QString levelText;
    switch (volumeLevel) {
        case 1: // Low
            volume = 0.3f;
            levelText = "Low";
            break;
        case 2: // Medium (Default)
            volume = 0.75f;  // Increased from 0.7 to 0.75
            levelText = "Medium";
            break;
        case 3: // High
            volume = 0.82f;   // Changed from 0.8 to 0.82
            levelText = "High";
            break;
    }
    
    qDebug() << "=== Volume Button Clicked ===";
    qDebug() << "Volume level changed: " << oldLevel << " -> " << volumeLevel 
             << " (" << levelText << ", value: " << volume << ")";
    
    // Set background music and sound effect volume
    SoundManager::getInstance()->setBackgroundVolume(volume);
    SoundManager::getInstance()->setEffectVolume(volume);
    
    // Update button image
    updateVolumeButton();
}

// Update volume button status
void MainWindow::updateVolumeButton()
{
    QPixmap volumeIcon = createVolumeImage(volumeLevel);
    volumeButton->setIcon(QIcon(volumeIcon));
    volumeButton->setIconSize(QSize(50, 50)); // Adjust icon size
    
    // Set border color to dark gray and add focus state
    QString buttonStyle = 
        "QPushButton { "
        "   background-color: rgba(255, 255, 255, 180); "
        "   border-radius: 10px; "
        "   border: 3px solid #444444; "
        "} "
        "QPushButton:hover { "
        "   background-color: rgba(255, 255, 255, 220); "
        "   border: 3px solid #4a86e8; "
        "} "
        "QPushButton:pressed { "
        "   background-color: rgba(220, 220, 220, 220); "
        "} "
        "QPushButton:focus { "
        "   border: 3px solid #444444; "
        "   outline: none; "
        "}";
    
    volumeButton->setStyleSheet(buttonStyle);
    
    // Set button to not receive focus
    volumeButton->setFocusPolicy(Qt::NoFocus);
    
    // Position is handled in updateVolumeButtonPosition() function
}

// Update volume button position (separated function)
void MainWindow::updateVolumeButtonPosition()
{
    if (volumeButton) {
        int margin = 15;
        volumeButton->move(width() - volumeButton->width() - margin, margin);
    }
}

// Pixel style volume icon creation
QPixmap MainWindow::createVolumeImage(int volumeLevel)
{
    QPixmap volumeImage(40, 40);
    volumeImage.fill(Qt::transparent);
    QPainter painter(&volumeImage);
    
    // Disable antialiasing (for pixel feel)
    painter.setRenderHint(QPainter::Antialiasing, false);
    
    // Common colors
    QColor darkGray(50, 50, 50);
    
    // Draw speaker body (always displayed)
    painter.setPen(Qt::NoPen);
    painter.setBrush(darkGray);
    
    // Speaker body (pixel style)
    painter.drawRect(8, 14, 8, 12); // Speaker rectangular body
    
    // Speaker front (triangle pixel style)
    painter.drawRect(16, 14, 2, 12); // Connector
    painter.drawRect(18, 12, 2, 16); // Front part 1
    painter.drawRect(20, 11, 2, 18); // Front part 2
    painter.drawRect(22, 10, 2, 20); // Front part 3
    
    // Show sound waves (represented by individual rectangles in pixel style)
    painter.setPen(Qt::NoPen);
    
    // First sound wave curve (always displayed, smallest curve)
    if (volumeLevel >= 1) {
        painter.drawRect(26, 15, 2, 2); // Top
        painter.drawRect(26, 23, 2, 2); // Bottom
        painter.drawRect(28, 13, 2, 2); // Top 2
        painter.drawRect(28, 25, 2, 2); // Bottom 2
        painter.drawRect(30, 14, 2, 12); // Vertical line
    }
    
    // Second sound wave curve (displayed in levels 2, 3)
    if (volumeLevel >= 2) {
        painter.drawRect(32, 10, 2, 2); // Top
        painter.drawRect(32, 28, 2, 2); // Bottom
        painter.drawRect(34, 8, 2, 2); // Top 2
        painter.drawRect(34, 30, 2, 2); // Bottom 2
        painter.drawRect(36, 10, 2, 20); // Vertical line
    }
    
    // Third sound wave curve (displayed only in level 3, largest curve)
    if (volumeLevel == 3) {
        painter.drawRect(38, 6, 2, 2); // Top
        painter.drawRect(38, 32, 2, 2); // Bottom
        painter.drawRect(40, 4, 2, 2); // Top 2
        painter.drawRect(40, 34, 2, 2); // Bottom 2
        painter.drawRect(42, 6, 2, 28); // Vertical line
    }
    
    // Add X mark for low volume (level 1)
    if (volumeLevel == 1) {
        painter.setBrush(Qt::red);
        // Pixel style X mark
        painter.drawRect(30, 16, 2, 2);
        painter.drawRect(32, 14, 2, 2);
        painter.drawRect(34, 16, 2, 2);
        painter.drawRect(32, 18, 2, 2);
        painter.drawRect(30, 20, 2, 2);
        painter.drawRect(34, 20, 2, 2);
        painter.drawRect(32, 22, 2, 2);
    }
    
    return volumeImage;
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    // 메인 메뉴 화면일 때만 배경 이미지 그리기
    if (stackedWidget->currentWidget() == mainMenu) {
        QPainter painter(this);
        
        // 배경 이미지 그리기
        if (!backgroundImage.isNull()) {
            // 이미지를 화면 전체 크기에 맞게 확장하여 그리기
            QPixmap scaledBg = backgroundImage.scaled(width(), height(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            
            // 이미지가 화면보다 크면 중앙 부분을 표시
            if (scaledBg.width() > width() || scaledBg.height() > height()) {
                int x = (scaledBg.width() - width()) / 2;
                int y = (scaledBg.height() - height()) / 2;
                painter.drawPixmap(0, 0, scaledBg, x, y, width(), height());
            } else {
                // 이미지가 화면보다 작으면 그냥 표시
                painter.drawPixmap(0, 0, scaledBg);
            }
        } else {
            // 이미지가 없는 경우 하늘색 배경
            painter.fillRect(rect(), QColor(135, 206, 235));
        }
    }
    
    // 부모 클래스의 paintEvent 호출
    QMainWindow::paintEvent(event);
}

MainWindow::~MainWindow()
{
    qDebug() << "DEBUG: MainWindow destructor called";
    
    // Safe memory deallocation
    if (colorCaptureWidget) {
        colorCaptureWidget->stopCameraCapture();
        delete colorCaptureWidget;
        colorCaptureWidget = nullptr;
    }
    
    if (bingoWidget) {
        delete bingoWidget;
        bingoWidget = nullptr;
    }
    
    if (multiGameWidget) {
        delete multiGameWidget;
        multiGameWidget = nullptr;
    }

    // Clean up sound resources
    SoundManager::getInstance()->cleanup();
    
    // Main menu and stacked widget will be automatically released by Qt
    
    qDebug() << "DEBUG: MainWindow destructor completed";

    delete matchingWidget;
    delete stackedWidget;
    delete bingoWidget;
    delete mainMenu;
}
