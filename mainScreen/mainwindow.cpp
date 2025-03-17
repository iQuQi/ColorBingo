#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QThread>
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

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
    
    qDebug() << "MainWindow 생성 완료, mainMenu 크기:" << mainMenu->size();
    qDebug() << "stackedWidget 크기:" << stackedWidget->size();
    qDebug() << "MainWindow 크기:" << size();
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
    
    // 타이틀 레이블
    QLabel *titleLabel = new QLabel("Color Bingo", centerWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 48px; font-weight: bold; color: #333; margin-bottom: 30px;");
    centerLayout->addWidget(titleLabel);
    
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
    multiGameButton->setStyleSheet(blueButtonStyle);
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
    
    if (!colorCaptureWidget) {
        qDebug() << "DEBUG: Creating new ColorCaptureWidget";
        colorCaptureWidget = new ColorCaptureWidget(this);
        connect(colorCaptureWidget, &ColorCaptureWidget::createBingoRequested, 
                this, &MainWindow::onCreateBingoRequested);
        connect(colorCaptureWidget, &ColorCaptureWidget::backToMainRequested, 
                this, &MainWindow::showMainMenu);
        
        // 스택 위젯에 추가
        stackedWidget->addWidget(colorCaptureWidget);
    }
    
    // 현재 위젯을 색상 캡처 위젯으로 변경
    stackedWidget->setCurrentWidget(colorCaptureWidget);
    qDebug() << "DEBUG: ColorCaptureWidget now displayed";
}

//<<<<<<< Updated upstream
// 멀티게임 화면 표시 (새로 추가)
void MainWindow::showMultiGameScreen()
{
    qDebug() << "DEBUG: Multi Game button clicked";
    
    if (!multiGameWidget) {
        qDebug() << "DEBUG: Creating new MultiGameWidget";
        multiGameWidget = new MultiGameWidget(this);
        connect(multiGameWidget, &MultiGameWidget::backToMainRequested, 
                this, &MainWindow::showMainMenu);
        
        // 스택 위젯에 추가
        stackedWidget->addWidget(multiGameWidget);
    }
    
    // 현재 위젯을 멀티게임 위젯으로 변경
    stackedWidget->setCurrentWidget(multiGameWidget);
    qDebug() << "DEBUG: MultiGameWidget now displayed";
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
    
    // BingoWidget 생성
    qDebug() << "DEBUG: Creating BingoWidget";
    bingoWidget = new BingoWidget(this, colors);
    
    // 시그널 연결
    connect(bingoWidget, &BingoWidget::backToMainRequested, 
            this, &MainWindow::showMainMenu, Qt::QueuedConnection);
    
    // 스택 위젯에 추가 및 현재 위젯으로 설정
//=======
   // bingoWidget = new BingoWidget(this);
   // matchingWidget = new MatchingWidget(this);

   // stackedWidget->addWidget(mainMenu);
   // stackedWidget->addWidget(matchingWidget);
//>>>>>>> Stashed changes
   // stackedWidget->addWidget(bingoWidget);
   // stackedWidget->setCurrentWidget(bingoWidget);
   // qDebug() << "DEBUG: BingoWidget now displayed";
}

//<<<<<<< Updated upstream
// 메인 메뉴 표시 (이전의 onBingoBackRequested)
void MainWindow::showMainMenu()
{
    qDebug() << "DEBUG: Back to main menu requested - SAFE HANDLING";
    
    // 이벤트 루프가 완료될 때까지 객체 삭제 지연
    QTimer::singleShot(0, this, [this]() {
        qDebug() << "DEBUG: Executing delayed widget cleanup";
        
        // 메인 메뉴로 전환
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

//void MainWindow::showMatchingScreen() {
//    stackedWidget->setCurrentWidget(matchingWidget);
//}

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
    
    // 메인 메뉴와 스택 위젯은 Qt가 자동으로 해제
    
    qDebug() << "DEBUG: MainWindow destructor completed";

    delete matchingWidget;
    delete stackedWidget;
    delete bingoWidget;
    delete mainMenu;
}
