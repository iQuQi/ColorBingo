#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent){
    stackedWidget = new QStackedWidget(this);

    mainMenu = new QWidget(this);
    QPushButton *startButton = new QPushButton("Single Game", this);
    QPushButton *multiGameButton = new QPushButton("Multi Game", this);
    QPushButton *exitButton = new QPushButton("Exit", this);

    QVBoxLayout *menuLayout = new QVBoxLayout();
    menuLayout->addWidget(startButton);
    menuLayout->addWidget(multiGameButton);
    menuLayout->addWidget(exitButton);
    mainMenu->setLayout(menuLayout);

    bingoWidget = new BingoWidget(this);
    multiGameWidget = new MultiGameWidget(this);

    stackedWidget->addWidget(mainMenu);
    stackedWidget->addWidget(bingoWidget);
    stackedWidget->addWidget(multiGameWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(stackedWidget);
    setLayout(mainLayout);
//    ui->setupUi(this);

    connect(startButton, &QPushButton::clicked, this, &MainWindow::showBingoScreen);
    connect(multiGameButton, &QPushButton::clicked, this, &MainWindow::showMultiGameScreen);
    connect(exitButton, &QPushButton::clicked, this, &QWidget::close);
    connect(bingoWidget, &BingoWidget::backToMainRequested, this, &MainWindow::showMainMenu);
    connect(multiGameWidget, &MultiGameWidget::backToMainRequested, this, &MainWindow::showMainMenu);
}

void MainWindow::showBingoScreen() {
    stackedWidget->setCurrentWidget(bingoWidget);
}

void MainWindow::showMultiGameScreen() {
    if (bingoWidget->isCameraCapturing()) {
        qDebug() << "Stopping BingoWidget camera before switching to MultiGameWidget.";
        bingoWidget->getCamera()->stopCapturing();
        bingoWidget->getCamera()->closeCamera();
    }

    stackedWidget->setCurrentWidget(multiGameWidget);
}

void MainWindow::showMainMenu() {
    stackedWidget->setCurrentWidget(mainMenu);
}

MainWindow::~MainWindow()
{
    delete stackedWidget;
    delete bingoWidget;
    delete multiGameWidget;
    delete mainMenu;
}
