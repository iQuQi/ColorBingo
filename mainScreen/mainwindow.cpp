#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QWidget(parent){
    stackedWidget = new QStackedWidget(this);

    mainMenu = new QWidget(this);
    QPushButton *startButton = new QPushButton("Start Bingo", this);
    QPushButton *exitButton = new QPushButton("Exit", this);

    QVBoxLayout *menuLayout = new QVBoxLayout();
    menuLayout->addWidget(startButton);
    menuLayout->addWidget(exitButton);
    mainMenu->setLayout(menuLayout);

    bingoWidget = new BingoWidget(this);

    stackedWidget->addWidget(mainMenu);
    stackedWidget->addWidget(bingoWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(stackedWidget);
    setLayout(mainLayout);
//    ui->setupUi(this);

    connect(startButton, &QPushButton::clicked, this, &MainWindow::showBingoScreen);
    connect(exitButton, &QPushButton::clicked, this, &QWidget::close);
}

void MainWindow::showBingoScreen() {
    stackedWidget->setCurrentWidget(bingoWidget);
}

MainWindow::~MainWindow()
{
    delete stackedWidget;
    delete bingoWidget;
    delete mainMenu;
}
