#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "hardwareInterface/SoundManager.h"
#include "ui/widgets/bingopreparationwidget.h"
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
#include <QFontDatabase>
#include <QApplication>
#include "utils/pixelartgenerator.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    colorCaptureWidget(nullptr),
    bingoWidget(nullptr),
    matchingWidget(nullptr),
    multiGameWidget(nullptr),
    volumeLevel(2), // Default volume level is 2 (medium)
    backgroundImage() // 배경 이미지 초기화 추가
{

    network = P2PNetwork::getInstance();

    connect(network, &P2PNetwork::opponentMultiGameReady, this, &MainWindow::onOpponentMultiGameReady);

    waitingLabel = new QLabel("Waiting for Other Player...", this);
    waitingLabel->setAlignment(Qt::AlignCenter);
    waitingLabel->setStyleSheet("background-color: rgba(0, 0, 0, 150); color: white; font-size: 24px; padding: 10px;");
    waitingLabel->setGeometry(50, 50, 400, 50);
    waitingLabel->hide();

    // 귀여운 폰트 설정
    int fontIdBold = QFontDatabase::addApplicationFont(":/fonts/ComicNeue-Bold.ttf");
    int fontIdRegular = QFontDatabase::addApplicationFont(":/fonts/ComicNeue-Regular.ttf");
    
    // 폰트 로딩 결과 확인
    QString fontFamily;
    if (fontIdBold != -1 && fontIdRegular != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontIdRegular);
        if (!fontFamilies.isEmpty()) {
            fontFamily = fontFamilies.at(0);
            qDebug() << "Successfully loaded font:" << fontFamily;
        }
    } else {
        qDebug() << "Failed to load Comic Neue fonts, falling back to system fonts";
        fontFamily = "Comic Sans MS";
    }
    
    // 전체 애플리케이션에 귀여운 폰트 적용
    QFont cuteFont(fontFamily, 10);
    QApplication::setFont(cuteFont);
    
    // 전체 앱에 폰트 패밀리 적용 (폰트 파일이 없는 경우 대체 폰트 사용)
    setStyleSheet(QString("* { font-family: '%1', 'Comic Sans MS', 'Segoe UI', 'Arial', sans-serif; }").arg(fontFamily));
    
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
    volumeButton->setCursor(Qt::PointingHandCursor);
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

void MainWindow::setupMainScreen()
{
    // Setup main screen widget
    QVBoxLayout *mainLayout = new QVBoxLayout(mainMenu);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Set background style - 배경 제거하고 투명하게 설정
    mainMenu->setStyleSheet("QWidget { background-color: transparent; }");
    
    // Create central container for content
    centerWidget = new QWidget(mainMenu);
    centerWidget->setStyleSheet("QWidget { background-color: rgba(255, 255, 255, 0); border-radius: 50px; }");
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
    QPixmap bearPixmap = PixelArtGenerator::getInstance()->createBearImage();
    
    // Left bear image label
    QLabel *leftBearLabel = new QLabel(centerWidget);
    leftBearLabel->setPixmap(bearPixmap);
    leftBearLabel->setFixedSize(bearPixmap.size());
    leftBearLabel->setStyleSheet("background-color: transparent;");
    
    // Title label
    QLabel *titleLabel = new QLabel("Color Bingo", centerWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    // 더 귀여운 스타일로 타이틀 폰트 수정 - 폰트 크기 더 증가 및 그림자 효과 제거
    titleLabel->setStyleSheet("font-size: 76px; font-weight: bold; color: #333; background-color: transparent; letter-spacing: 2px;");
    
    // Right bear image label
    QLabel *rightBearLabel = new QLabel(centerWidget);
    rightBearLabel->setPixmap(bearPixmap);
    rightBearLabel->setFixedSize(bearPixmap.size());
    rightBearLabel->setStyleSheet("background-color: transparent;");
    
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
    
    // 현재 불러온 폰트 패밀리 가져오기
    QString fontFamily = QApplication::font().family();
    
    // PixelArtGenerator를 사용하여 픽셀 스타일 버튼 스타일 생성
    PixelArtGenerator* pixelArtGen = PixelArtGenerator::getInstance();
    
    // 각 버튼에 픽셀 스타일 적용
    QString blueButtonStyle = pixelArtGen->createPixelButtonStyle(QColor("#4a86e8"));
    QString greenButtonStyle = pixelArtGen->createPixelButtonStyle(QColor("#8BC34A"));
    QString redButtonStyle = pixelArtGen->createPixelButtonStyle(QColor("#e74c3c"));
    
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
    
    // 기존 colorCaptureWidget이 없으면 생성
    if (!colorCaptureWidget) {
        qDebug() << "DEBUG: Creating new BingoPreparationWidget";
        colorCaptureWidget = new BingoPreparationWidget(this);
        // Add to stacked widget
        stackedWidget->addWidget(colorCaptureWidget);
    }
    
    // 단일 게임 모드 설정
    colorCaptureWidget->setGameMode(GameMode::SINGLE);
    qDebug() << "DEBUG: BingoPreparationWidget set to SINGLE mode";
    
    // Signal/slot connections
    qDebug() << "DEBUG: Reconnecting signals for Single Game mode";
    disconnect(colorCaptureWidget, nullptr, this, nullptr); // Disconnect all existing connections
    connect(colorCaptureWidget, &BingoPreparationWidget::createBingoRequested, 
            this, &MainWindow::onCreateBingoRequested);
    connect(colorCaptureWidget, &BingoPreparationWidget::backToMainRequested, 
            this, &MainWindow::showMainMenu);
    
    // Show color capture widget
    stackedWidget->setCurrentWidget(colorCaptureWidget);
    qDebug() << "DEBUG: BingoPreparationWidget now displayed";
}


// 매칭 화면 표시 (멀티게임 버튼 클릭 시, 멀티게임 화면 표시 전)
//void MainWindow::showMatchingScreen() {
//    if (!matchingWidget) {
//         qDebug() << "DEBUG: Creating new MatchingWidget";
//         matchingWidget = new MatchingWidget(this);
//         stackedWidget->addWidget(matchingWidget);
//         connect(matchingWidget, &MatchingWidget::switchToBingoScreen, this, &MainWindow::showMultiGameScreen);
//    }
//    qDebug() << "DEBUG: Multi Game button clicked";

//    stackedWidget->setCurrentWidget(matchingWidget);
//}
void MainWindow::showMatchingScreen() {
    qDebug() << "DEBUG: Navigating to Matching Screen";

    P2PNetwork::getInstance()->isMatchingActive = false;

    // ✅ 기존 `MatchingWidget`이 있으면 삭제 후 새로 생성
    if (matchingWidget) {
        stackedWidget->removeWidget(matchingWidget);
        delete matchingWidget;
        matchingWidget = nullptr;
    }

    // ✅ 새로운 매칭 위젯 생성
    matchingWidget = new MatchingWidget(this);

    // ✅ Back 버튼을 눌렀을 때 메인 화면으로 돌아가도록 연결
    connect(matchingWidget, &MatchingWidget::backToMainRequested, this, &MainWindow::showMainMenu);
    connect(matchingWidget, &MatchingWidget::switchToBingoScreen, this, &MainWindow::showMultiGameScreen);

    // ✅ 스택 위젯에 추가하고 매칭 화면으로 전환
    stackedWidget->addWidget(matchingWidget);
    stackedWidget->setCurrentWidget(matchingWidget);

    // ✅ 매칭 시작
//    matchingWidget->startMatching();
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

    // 기존 colorCaptureWidget이 없으면 생성
    if (!colorCaptureWidget) {
        qDebug() << "DEBUG: Creating new BingoPreparationWidget";
        colorCaptureWidget = new BingoPreparationWidget(this);
        // Add to stacked widget
        stackedWidget->addWidget(colorCaptureWidget);
    }
    
    // 멀티 게임 모드 설정
    colorCaptureWidget->setGameMode(GameMode::MULTI);
    qDebug() << "DEBUG: BingoPreparationWidget set to MULTI mode";
    
    // Signal/slot connections
    qDebug() << "DEBUG: Reconnecting signals for Multi Game mode";
    disconnect(colorCaptureWidget, nullptr, this, nullptr); // Disconnect all existing connections
    connect(colorCaptureWidget, &BingoPreparationWidget::createMultiGameRequested,
            this, &MainWindow::onCreateMultiGameRequested);
    connect(colorCaptureWidget, &BingoPreparationWidget::backToMainRequested,
            this, &MainWindow::showMainMenu);
    
    // Show color capture widget
    stackedWidget->setCurrentWidget(colorCaptureWidget);
    qDebug() << "DEBUG: BingoPreparationWidget now displayed";
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
    isLocalMultiGameReady = true;
    storedColors = colors;
    qDebug() << "DEBUG: Local board requested multi-game with" << colors.size() << "colors";

    // 상대 보드에게 준비 완료 메시지 전송
    network->sendMultiGameReady();

    // 상대방이 아직 준비되지 않았다면 "Waiting for Other Player"
    if (!isOpponentMultiGameReady) {
        qDebug() << "DEBUG: Waiting for other player to be ready";
        //waitingLabel->show();
    }

    checkIfBothPlayersReady();
}

void MainWindow::onOpponentMultiGameReady() {
    isOpponentMultiGameReady = true;
    qDebug() << "✅ Opponent board requested multi-game";

    // 상대방이 준비되었으므로 "Waiting for Other Player" 메시지 숨김
    //waitingLabel->hide();

    // 양쪽 보드가 모두 준비되었는지 확인
    checkIfBothPlayersReady();
}


void MainWindow::checkIfBothPlayersReady() {
    // ✅ 이미 실행된 경우 중복 실행 방지
    if (isMultiGameStarted) {
        qDebug() << "⚠️ Multi-game already started. Skipping duplicate call.";
        return;
    }

    if (isLocalMultiGameReady && isOpponentMultiGameReady) {
        qDebug() << "DEBUG: 🎮 Both players are ready! Moving to MultiGame screen.";

        // ✅ 중복 실행 방지 플래그 설정
        isMultiGameStarted = true;

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
        multiGameWidget = new MultiGameWidget(this, storedColors);

        // Connect signals
        connect(multiGameWidget, &MultiGameWidget::backToMainRequested,
                this, &MainWindow::showMainMenu, Qt::QueuedConnection);

        // Add to stacked widget and set as current widget
        stackedWidget->addWidget(multiGameWidget);
        stackedWidget->setCurrentWidget(multiGameWidget);
        qDebug() << "DEBUG: MultiGameWidget now displayed";
    }
}

// Display main menu (previously onBingoBackRequested)
void MainWindow::showMainMenu()
{
    qDebug() << "DEBUG: Back to main menu requested - SAFE HANDLING";
    
    network->disconnectFromPeer();

    // Ensure widget camera resources are released
    if (colorCaptureWidget && stackedWidget->currentWidget() == colorCaptureWidget) {
        qDebug() << "DEBUG: Stopping BingoPreparationWidget camera before returning to main menu";
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
    
    // ✅ P2P 네트워크 연결 해제 (매칭 중단)
    P2PNetwork::getInstance()->disconnectFromPeer();

    // ✅ 현재 매칭 위젯 삭제
    if (matchingWidget) {
        stackedWidget->removeWidget(matchingWidget);
        delete matchingWidget;
        matchingWidget = nullptr;
    }

    // Delay object deletion until event loop completes
    QTimer::singleShot(0, this, [this]() {
        qDebug() << "DEBUG: Executing delayed widget cleanup";
        
        // Switch to main menu
        stackedWidget->setCurrentWidget(mainMenu);
        qDebug() << "DEBUG: Main menu now displayed";
    });

    isLocalMultiGameReady = false;
    isOpponentMultiGameReady = false;
    isMultiGameStarted = false;

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
    QPixmap volumeIcon = PixelArtGenerator::getInstance()->createVolumeImage(volumeLevel);
    volumeButton->setIcon(QIcon(volumeIcon));
    volumeButton->setIconSize(QSize(50, 50)); // Adjust icon size
    
    // 픽셀 스타일 버튼 적용 (흰색으로 설정하여 focus 상태와 상관없이 항상 흰색 유지)
    QString buttonStyle = PixelArtGenerator::getInstance()->createPixelButtonStyle(
        QColor(255, 255, 255, 200), // 반투명 흰색
        3,  // 테두리 두께
        10  // 둥글기 정도
    );
    
    // focus 상태에서도 동일한 색상을 유지하기 위해 추가 스타일 적용
    buttonStyle += "QPushButton:focus { background-color: rgba(255, 255, 255, 200); }"
                   "QPushButton:hover { background-color: rgba(255, 255, 255, 220); }"
                   "QPushButton:pressed { background-color: rgba(255, 255, 255, 240); }";
    
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

    if(matchingWidget){
        delete matchingWidget;
        matchingWidget = nullptr;
    }

    // Clean up sound resources
    SoundManager::getInstance()->cleanup();
    
    qDebug() << "DEBUG: MainWindow destructor completed";
}
