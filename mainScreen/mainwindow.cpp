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
    bingoWidget(nullptr)
{
    // 스택 위젯 초기화
    stackedWidget = new QStackedWidget(this);
    
    // 메인 메뉴 화면 초기화
    mainMenu = new QWidget(this);
    
    // 메인 화면 초기화
    setupMainScreen();
    
    // 중앙 위젯으로 스택 위젯 설정
    setCentralWidget(stackedWidget);
    
    // 메인 메뉴를 스택 위젯에 추가
    stackedWidget->addWidget(mainMenu);
    stackedWidget->setCurrentWidget(mainMenu);
}

void MainWindow::setupMainScreen()
{
    // 메인 화면 위젯 설정
    QVBoxLayout *mainLayout = new QVBoxLayout(mainMenu);
    
    // 컨텐츠를 담을 중앙 컨테이너 생성
    centerWidget = new QWidget(mainMenu);
    centerWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    QVBoxLayout *centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setSpacing(20);
    centerLayout->setContentsMargins(20, 20, 20, 20);
    
    // 타이틀 레이블
    QLabel *titleLabel = new QLabel("Color Bingo", centerWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 48px; font-weight: bold; color: #333; margin-bottom: 30px;");
    centerLayout->addWidget(titleLabel);
    
    // 버튼 공통 크기 설정
    const int BUTTON_WIDTH = 280;
    const int BUTTON_HEIGHT = 70;
    
    // Start Bingo 버튼
    QPushButton *startBingoButton = new QPushButton("Start Bingo", centerWidget);
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
    QPushButton *exitButton = new QPushButton("Exit", centerWidget);
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
    centerWidget->adjustSize();
    
    // 메인 레이아웃에 스트레치와 중앙 컨테이너 추가
    mainLayout->addStretch(1);
    mainLayout->addWidget(centerWidget, 0, Qt::AlignCenter);
    mainLayout->addStretch(1);
    
    // 시그널 연결
    connect(startBingoButton, &QPushButton::clicked, this, &MainWindow::onStartBingoClicked);
    connect(exitButton, &QPushButton::clicked, this, &QMainWindow::close);
    
    // 중앙 위젯 위치 업데이트
    updateCenterWidgetPosition();
}

void MainWindow::updateCenterWidgetPosition()
{
    if (mainMenu && centerWidget) {
        int x = (mainMenu->width() - centerWidget->width()) / 2;
        int y = (mainMenu->height() - centerWidget->height()) / 2;
        centerWidget->move(x, y);
    }
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
        
        // 스택 위젯에 추가
        stackedWidget->addWidget(colorCaptureWidget);
    }
    
    // 현재 위젯을 색상 캡처 위젯으로 변경
    stackedWidget->setCurrentWidget(colorCaptureWidget);
    qDebug() << "DEBUG: ColorCaptureWidget now displayed";
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
            this, &MainWindow::onBingoBackRequested, Qt::QueuedConnection);
    
    // 스택 위젯에 추가 및 현재 위젯으로 설정
    stackedWidget->addWidget(bingoWidget);
    stackedWidget->setCurrentWidget(bingoWidget);
    qDebug() << "DEBUG: BingoWidget now displayed";
}

void MainWindow::onBingoBackRequested()
{
    qDebug() << "DEBUG: Back to main requested - SAFE HANDLING";
    
    // 이벤트 루프가 완료될 때까지 객체 삭제 지연
    QTimer::singleShot(0, this, [this]() {
        qDebug() << "DEBUG: Executing delayed BingoWidget cleanup";
        
        // 메인 메뉴로 전환
        stackedWidget->setCurrentWidget(mainMenu);
        
        // bingoWidget을 안전하게 정리
        if (bingoWidget) {
            qDebug() << "DEBUG: Cleaning up BingoWidget";
            disconnect(bingoWidget);
            stackedWidget->removeWidget(bingoWidget);
            bingoWidget->deleteLater();
            bingoWidget = nullptr;
        }
        
        qDebug() << "DEBUG: Returned to main screen safely";
    });
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
    
    // 메인 메뉴와 스택 위젯은 Qt가 자동으로 해제
    
    qDebug() << "DEBUG: MainWindow destructor completed";
}
