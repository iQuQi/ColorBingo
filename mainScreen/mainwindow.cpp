#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "hardwareInterface/SoundManager.h"
#include "ui/widgets/colorcapturewidget.h"
#include <QDebug>
#include <QThread>
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    colorCaptureWidget(nullptr),
    bingoWidget(nullptr),
    multiGameWidget(nullptr)
{
    // 기본 창 크기 설정
    resize(800, 600);
    
    // 스택 위젯 초기화
    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);
    
    // 메인 메뉴 화면 초기화
    mainMenu = new QWidget();
    
    // 메인 화면 초기화 (mainMenu에 내용 추가)
    setupMainScreen();
    
    // 메인 메뉴를 스택 위젯에 추가
    stackedWidget->addWidget(mainMenu);
    stackedWidget->setCurrentWidget(mainMenu);

    // 배경음악 시작
    SoundManager::getInstance()->playBackgroundMusic();
    
    qDebug() << "MainWindow 생성 완료, mainMenu 크기:" << mainMenu->size();
    qDebug() << "stackedWidget 크기:" << stackedWidget->size();
    qDebug() << "MainWindow 크기:" << size();
}

// 픽셀 스타일의 곰돌이 이미지 생성 함수
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
    // 메인 화면 위젯 설정
    QVBoxLayout *mainLayout = new QVBoxLayout(mainMenu);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 배경 스타일 설정
    mainMenu->setStyleSheet("QWidget { background-color: #f5f5f5; }");
    
    // 컨텐츠를 담을 중앙 컨테이너 생성
    centerWidget = new QWidget(mainMenu);
    centerWidget->setStyleSheet("QWidget { background-color: #ffffff; border-radius: 10px; }");
    centerWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    QVBoxLayout *centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setSpacing(20);
    centerLayout->setContentsMargins(30, 30, 30, 30);
    
    // 타이틀 영역을 위한 수평 레이아웃 생성
    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->setAlignment(Qt::AlignVCenter); // 모든 요소를 수직 중앙에 정렬
    
    // 곰돌이 이미지 생성
    QPixmap bearPixmap = createBearImage();
    
    // 왼쪽 곰돌이 이미지 레이블
    QLabel *leftBearLabel = new QLabel(centerWidget);
    leftBearLabel->setPixmap(bearPixmap);
    leftBearLabel->setFixedSize(bearPixmap.size());
    
    // 타이틀 레이블
    QLabel *titleLabel = new QLabel("Color Bingo", centerWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    // 하단 마진 제거하고 폰트 크기만 유지
    titleLabel->setStyleSheet("font-size: 48px; font-weight: bold; color: #333;");
    
    // 오른쪽 곰돌이 이미지 레이블
    QLabel *rightBearLabel = new QLabel(centerWidget);
    rightBearLabel->setPixmap(bearPixmap);
    rightBearLabel->setFixedSize(bearPixmap.size());
    
    // 레이아웃에 위젯 추가 (모두 수직 중앙 정렬로 설정)
    titleLayout->addWidget(leftBearLabel, 0, Qt::AlignVCenter);
    titleLayout->addWidget(titleLabel, 1, Qt::AlignVCenter); // 1은 stretch factor, 더 많은 공간을 차지하게 함
    titleLayout->addWidget(rightBearLabel, 0, Qt::AlignVCenter);
    
    // 타이틀 레이아웃에 위아래 여백 추가 (전체적으로 간격 유지)
    titleLayout->setContentsMargins(0, 0, 0, 20);
    
    // 타이틀 레이아웃을 메인 레이아웃에 추가
    centerLayout->addLayout(titleLayout);
    
    // 버튼 공통 크기 설정
    const int BUTTON_WIDTH = 280;
    const int BUTTON_HEIGHT = 70;
    
    // 버튼 공통 스타일 
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
    
    // 파스텔 연초록 스타일 추가
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
    
    // Single Game 버튼 (이전의 Start Bingo 버튼)
    QPushButton *singleGameButton = new QPushButton("Single Game", centerWidget);
    singleGameButton->setStyleSheet(blueButtonStyle);
    singleGameButton->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    singleGameButton->setCursor(Qt::PointingHandCursor);
    centerLayout->addWidget(singleGameButton, 0, Qt::AlignCenter);
    
    // Multi Game 버튼 (새로 추가)
    QPushButton *multiGameButton = new QPushButton("Multi Game", centerWidget);
    multiGameButton->setStyleSheet(greenButtonStyle);
    multiGameButton->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    multiGameButton->setCursor(Qt::PointingHandCursor);
    centerLayout->addWidget(multiGameButton, 0, Qt::AlignCenter);
    
    // Exit 버튼
    QPushButton *exitButton = new QPushButton("Exit", centerWidget);
    exitButton->setStyleSheet(redButtonStyle);
    exitButton->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    exitButton->setCursor(Qt::PointingHandCursor);
    centerLayout->addWidget(exitButton, 0, Qt::AlignCenter);
    
    // 컨테이너 크기 조정
    centerWidget->adjustSize();
    
    // 메인 레이아웃에 스트레치와 중앙 컨테이너 추가
    mainLayout->addStretch(1);
    mainLayout->addWidget(centerWidget, 0, Qt::AlignCenter);
    mainLayout->addStretch(1);
    
    // 시그널 연결
    connect(singleGameButton, &QPushButton::clicked, this, &MainWindow::showBingoScreen);
    connect(multiGameButton, &QPushButton::clicked, this, &MainWindow::showMultiGameScreen);
    connect(exitButton, &QPushButton::clicked, this, &QMainWindow::close);
    
    qDebug() << "setupMainScreen 완료, centerWidget 크기:" << centerWidget->size();
}

// 단일 게임 화면 표시 (이전의 onStartBingoClicked)
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
        
        // 카메라 리소스 완전 해제를 위한 대기 시간
        QThread::msleep(500);
    }
    
    if (!colorCaptureWidget) {
        qDebug() << "DEBUG: Creating new ColorCaptureWidget";
        colorCaptureWidget = new ColorCaptureWidget(this);
        // 스택 위젯에 추가
        stackedWidget->addWidget(colorCaptureWidget);
    }
    
    // 단일 게임 모드로 설정
    colorCaptureWidget->setGameMode(GameMode::SINGLE);
    qDebug() << "DEBUG: ColorCaptureWidget set to SINGLE mode";
    
    // 기존 연결 해제 후 새로운 연결 설정
    qDebug() << "DEBUG: Reconnecting signals for Single Game mode";
    disconnect(colorCaptureWidget, nullptr, this, nullptr); // 기존 모든 연결 해제
    connect(colorCaptureWidget, &ColorCaptureWidget::createBingoRequested, 
            this, &MainWindow::onCreateBingoRequested);
    connect(colorCaptureWidget, &ColorCaptureWidget::backToMainRequested, 
            this, &MainWindow::showMainMenu);
    
    // 현재 위젯을 색상 캡처 위젯으로 변경
    stackedWidget->setCurrentWidget(colorCaptureWidget);
    
    // 리소스가 완전히 초기화될 시간을 주기 위해 짧은 지연 추가
    QTimer::singleShot(500, this, [this]() {
        qDebug() << "DEBUG: ColorCaptureWidget now displayed";
    });
}

// 멀티게임 화면 표시 (새로 추가)
void MainWindow::showMultiGameScreen()
{
    qDebug() << "DEBUG: Multi Game button clicked";

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

        // 카메라 리소스 완전 해제를 위한 대기 시간
        QThread::msleep(500);
    }

    if (!colorCaptureWidget) {
        qDebug() << "DEBUG: Creating new ColorCaptureWidget";
        colorCaptureWidget = new ColorCaptureWidget(this);
        // 스택 위젯에 추가
        stackedWidget->addWidget(colorCaptureWidget);
    }

    // 멀티 게임 모드로 설정
    colorCaptureWidget->setGameMode(GameMode::MULTI);
    qDebug() << "DEBUG: ColorCaptureWidget set to MULTI mode";
    
    // 기존 연결 해제 후 새로운 연결 설정
    qDebug() << "DEBUG: Reconnecting signals for Multi Game mode";
    disconnect(colorCaptureWidget, nullptr, this, nullptr); // 기존 모든 연결 해제
    connect(colorCaptureWidget, &ColorCaptureWidget::createMultiGameRequested,
            this, &MainWindow::onCreateMultiGameRequested);
    connect(colorCaptureWidget, &ColorCaptureWidget::backToMainRequested,
            this, &MainWindow::showMainMenu);

    // 현재 위젯을 색상 캡처 위젯으로 변경
    stackedWidget->setCurrentWidget(colorCaptureWidget);

    // 리소스가 완전히 초기화될 시간을 주기 위해 짧은 지연 추가
    QTimer::singleShot(500, this, [this]() {
        qDebug() << "DEBUG: ColorCaptureWidget now displayed";
    });
}

void MainWindow::onCreateBingoRequested(const QList<QColor> &colors)
{
    qDebug() << "DEBUG: Create Bingo requested with" << colors.size() << "colors";
    
    // 카메라 리소스 해제 - 완전히 닫기
    if (colorCaptureWidget) {
        qDebug() << "DEBUG: Stopping camera before creating BingoWidget";
        colorCaptureWidget->stopCameraCapture();
        
        // 카메라 자원 해제를 위한 대기 시간
        qDebug() << "DEBUG: Waiting for camera resources to be fully released";
        QThread::msleep(1500); // 1.5초 대기
    }
    
    // 기존 bingoWidget이 있다면 안전하게 정리
    if (bingoWidget) {
        qDebug() << "DEBUG: Cleaning up previous BingoWidget";
        disconnect(bingoWidget); // 시그널 연결 해제
        stackedWidget->removeWidget(bingoWidget);
        delete bingoWidget;
        bingoWidget = nullptr;
    }

    if (multiGameWidget) {
        qDebug() << "DEBUG: Cleaning up previous MultiGameWidget";
        disconnect(multiGameWidget); // 시그널 연결 해제
        stackedWidget->removeWidget(multiGameWidget);
        delete multiGameWidget;
        multiGameWidget = nullptr;
    }
    
    // BingoWidget 생성
    qDebug() << "DEBUG: Creating BingoWidget";
    bingoWidget = new BingoWidget(this, colors);
    
    // 시그널 연결
    connect(bingoWidget, &BingoWidget::backToMainRequested,
            this, &MainWindow::showMainMenu, Qt::QueuedConnection);
    
    // 스택 위젯에 추가 및 현재 위젯으로 설정
    stackedWidget->addWidget(bingoWidget);
    stackedWidget->setCurrentWidget(bingoWidget);
    qDebug() << "DEBUG: BingoWidget now displayed";
}

void MainWindow::onCreateMultiGameRequested(const QList<QColor> &colors)
{
    qDebug() << "DEBUG: Create Multi Game requested with" << colors.size() << "colors";

    // 카메라 리소스 해제 - 완전히 닫기
    if (colorCaptureWidget) {
        qDebug() << "DEBUG: Stopping camera before creating MultiGameWidget";
        colorCaptureWidget->stopCameraCapture();

        // 카메라 자원 해제를 위한 대기 시간
        qDebug() << "DEBUG: Waiting for camera resources to be fully released";
        QThread::msleep(1500); // 1.5초 대기
    }

    // 기존 bingoWidget이 있다면 안전하게 정리
    if (bingoWidget) {
        qDebug() << "DEBUG: Cleaning up previous BingoWidget";
        disconnect(bingoWidget); // 시그널 연결 해제
        stackedWidget->removeWidget(bingoWidget);
        delete bingoWidget;
        bingoWidget = nullptr;
    }

    if (multiGameWidget) {
        qDebug() << "DEBUG: Cleaning up previous MultiGameWidget";
        disconnect(multiGameWidget); // 시그널 연결 해제
        stackedWidget->removeWidget(multiGameWidget);
        delete multiGameWidget;
        multiGameWidget = nullptr;
    }

    // BingoWidget 생성
    qDebug() << "DEBUG: Creating MultiGameWidget";
    multiGameWidget = new MultiGameWidget(this, colors);

    // 시그널 연결
    connect(multiGameWidget, &MultiGameWidget::backToMainRequested,
            this, &MainWindow::showMainMenu, Qt::QueuedConnection);

    // 스택 위젯에 추가 및 현재 위젯으로 설정
    stackedWidget->addWidget(multiGameWidget);
    stackedWidget->setCurrentWidget(multiGameWidget);
    qDebug() << "DEBUG: MultiGameWidget now displayed";
}

// 메인 메뉴 표시 (이전의 onBingoBackRequested)
void MainWindow::showMainMenu()
{
    qDebug() << "DEBUG: Back to main menu requested - SAFE HANDLING";
    
    // 위젯들의 카메라 리소스 확실히 해제
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
    
    // 이벤트 루프가 완료될 때까지 객체 삭제 지연
    QTimer::singleShot(0, this, [this]() {
        qDebug() << "DEBUG: Executing delayed widget cleanup";
        
        // 메인 메뉴로 전환
        stackedWidget->setCurrentWidget(mainMenu);
        qDebug() << "DEBUG: Main menu now displayed";
    });
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        updateCenterWidgetPosition();
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

MainWindow::~MainWindow()
{
    qDebug() << "DEBUG: MainWindow destructor called";
    
    // 안전한 메모리 해제
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

    // 사운드 리소스 정리
    SoundManager::getInstance()->cleanup();
    
    // 메인 메뉴와 스택 위젯은 Qt가 자동으로 해제
    
    qDebug() << "DEBUG: MainWindow destructor completed";
}
