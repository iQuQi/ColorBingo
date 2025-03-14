#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QThread>
#include <QRandomGenerator>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    mainScreen(nullptr),
    colorCaptureWidget(nullptr),
    bingoWidget(nullptr)
{
    // 메인 화면 초기화
    setupMainScreen();
}

void MainWindow::setupMainScreen()
{
    // 기존 메인 화면이 있다면 삭제
    if (mainScreen) {
        delete mainScreen;
    }

    // 새 메인 화면 생성
    mainScreen = new QWidget(this);
    
    // 배경 스타일 설정
    mainScreen->setStyleSheet("QWidget { background-color: #f5f5f5; }");
    
    // 중앙에 배치될 컨테이너 위젯
    QWidget *centerContainer = new QWidget(mainScreen);
    QVBoxLayout *centerLayout = new QVBoxLayout(centerContainer);
    centerLayout->setSpacing(20);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    
    // 타이틀 레이블
    QLabel *titleLabel = new QLabel("Color Bingo", centerContainer);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 48px; font-weight: bold; color: #333; margin-bottom: 40px;");
    centerLayout->addWidget(titleLabel);
    
    // 버튼 공통 크기 설정
    const int BUTTON_WIDTH = 280;
    const int BUTTON_HEIGHT = 70;
    
    // Start Bingo 버튼
    QPushButton *startBingoButton = new QPushButton("Start Bingo", centerContainer);
    startBingoButton->setStyleSheet(
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
        "}"
    );
    startBingoButton->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    startBingoButton->setCursor(Qt::PointingHandCursor);
    centerLayout->addWidget(startBingoButton, 0, Qt::AlignCenter);
    
    // Exit 버튼
    QPushButton *exitButton = new QPushButton("Exit", centerContainer);
    exitButton->setStyleSheet(
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
        "}"
    );
    exitButton->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    exitButton->setCursor(Qt::PointingHandCursor);
    centerLayout->addWidget(exitButton, 0, Qt::AlignCenter);
    
    // 컨테이너 크기 조정
    centerContainer->adjustSize();
    
    // 시그널 연결
    connect(startBingoButton, &QPushButton::clicked, this, &MainWindow::onStartBingoClicked);
    connect(exitButton, &QPushButton::clicked, this, &QMainWindow::close);
    
    // 메인 화면으로 설정
    setCentralWidget(mainScreen);
    
    // 리사이즈 이벤트 처리를 위해 이벤트 필터 설치
    mainScreen->installEventFilter(this);
    
    // centerContainer 포인터를 멤버 변수로 저장
    if (centerWidget) {
        delete centerWidget;
    }
    centerWidget = centerContainer;
    
    // 초기 위치 설정
    updateCenterWidgetPosition();
}

// 새로운 멤버 함수 추가 - 중앙 위젯 위치 업데이트
void MainWindow::updateCenterWidgetPosition()
{
    if (mainScreen && centerWidget) {
        int x = (mainScreen->width() - centerWidget->width()) / 2;
        int y = (mainScreen->height() - centerWidget->height()) / 2;
        centerWidget->move(x, y);
    }
}

// 이벤트 필터 구현
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == mainScreen && event->type() == QEvent::Resize) {
        updateCenterWidgetPosition();
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onStartBingoClicked()
{
    qDebug() << "DEBUG: Start Bingo button clicked";
    
    if (!colorCaptureWidget) {
        qDebug() << "DEBUG: Creating new ColorCaptureWidget";
        colorCaptureWidget = new ColorCaptureWidget(this);
        connect(colorCaptureWidget, &ColorCaptureWidget::createBingoRequested, 
                this, &MainWindow::onCreateBingoRequested);
        connect(colorCaptureWidget, &ColorCaptureWidget::backToMainRequested, 
                this, &MainWindow::onBingoBackRequested);
    }
    
    setCentralWidget(colorCaptureWidget);
    qDebug() << "DEBUG: ColorCaptureWidget now displayed";
}

void MainWindow::onCreateBingoRequested(const QList<QColor> &colors)
{
    qDebug() << "DEBUG: Create Bingo requested with" << colors.size() << "colors";
    
    // 카메라 리소스 해제 - 완전히 닫기
    if (colorCaptureWidget) {
        qDebug() << "DEBUG: Stopping camera before creating BingoWidget";
        colorCaptureWidget->stopCameraCapture();
        
        // 카메라 자원 해제를 위한 대기 시간 증가
        qDebug() << "DEBUG: Waiting for camera resources to be fully released";
        QThread::msleep(1500); // 1.5초 대기로 증가
    }
    
    // 기존 bingoWidget이 있다면 안전하게 정리
    if (bingoWidget) {
        qDebug() << "DEBUG: Cleaning up previous BingoWidget";
        disconnect(bingoWidget); // 시그널 연결 해제
        bingoWidget->deleteLater();
        bingoWidget = nullptr;
    }
    
    // 이벤트 루프 하나가 완료된 후에 BingoWidget 생성
    QTimer::singleShot(0, this, [this, colors]() {
        qDebug() << "DEBUG: Creating new BingoWidget safely";
        
        // BingoWidget 생성
        bingoWidget = new BingoWidget(this, colors);
        
        // 시그널 연결 - 안전하게
        connect(bingoWidget, &BingoWidget::backToMainRequested, 
                this, &MainWindow::onBingoBackRequested, Qt::QueuedConnection);
        
        // 중앙 위젯 설정
        setCentralWidget(bingoWidget);
        qDebug() << "DEBUG: BingoWidget now displayed safely";
    });
}

void MainWindow::onBingoBackRequested()
{
    qDebug() << "DEBUG: Back to main requested - SAFE HANDLING";
    
    // 이벤트 루프가 완료될 때까지 객체 삭제 지연
    QTimer::singleShot(0, this, [this]() {
        qDebug() << "DEBUG: Executing delayed BingoWidget cleanup";
        
        // 현재 중앙 위젯 제거 (삭제하지 않고)
        if (centralWidget()) {
            centralWidget()->setParent(nullptr);
        }
        
        // 메인 화면 설정
        setupMainScreen();
        qDebug() << "DEBUG: Main screen setup complete";
        
        // bingoWidget을 nullptr로 설정하여 나중에 참조되지 않도록 함
        if (bingoWidget) {
            qDebug() << "DEBUG: Cleaning up BingoWidget";
            bingoWidget->deleteLater(); // 안전하게 삭제
            bingoWidget = nullptr;
        }
        
        qDebug() << "DEBUG: Returned to main screen safely";
    });
}

MainWindow::~MainWindow()
{
    qDebug() << "DEBUG: MainWindow destructor called";
    
    // 모든 위젯 연결 및 시그널 끊기
    disconnect();
    
    // 안전한 메모리 해제 - deleteLater 사용
    if (colorCaptureWidget) {
        colorCaptureWidget->stopCameraCapture();
        colorCaptureWidget->deleteLater();
        colorCaptureWidget = nullptr;
    }
    
    if (bingoWidget) {
        bingoWidget->deleteLater();
        bingoWidget = nullptr;
    }
    
    // mainScreen은 centralWidget으로 관리되므로 수동 삭제 불필요
    
    qDebug() << "DEBUG: MainWindow destructor completed";
}
