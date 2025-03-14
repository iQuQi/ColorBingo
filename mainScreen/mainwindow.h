#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QStackedWidget>
#include <QList>
#include <QColor>
#include "colorcapturewidget.h"
#include "bingowidget.h"
#include "multigamewidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void showBingoScreen();
    void showMultiGameScreen();
    void showMainMenu();
    void onCreateBingoRequested(const QList<QColor> &colors);

private:
    void setupMainScreen();
    void updateCenterWidgetPosition();
    bool event(QEvent *event) override;
    
    QStackedWidget *stackedWidget;
    QWidget *mainMenu;
    QWidget *centerWidget = nullptr;
    ColorCaptureWidget *colorCaptureWidget;
    BingoWidget *bingoWidget;
    MultiGameWidget *multiGameWidget;
};

#endif // MAINWINDOW_H
