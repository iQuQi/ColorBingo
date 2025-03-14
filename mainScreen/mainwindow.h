#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QStackedWidget>
#include "colorcapturewidget.h"
#include "bingowidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartBingoClicked();
    void onCreateBingoRequested(const QList<QColor> &colors);
    void onBingoBackRequested();

private:
    void setupMainScreen();
    void updateCenterWidgetPosition();
    
    QStackedWidget *stackedWidget;
    QWidget *mainMenu;
    QWidget *centerWidget = nullptr;
    ColorCaptureWidget *colorCaptureWidget;
    BingoWidget *bingoWidget;
};

#endif // MAINWINDOW_H
