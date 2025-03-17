#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QStackedWidget>
#include <QList>
#include <QColor>
#include <QPixmap>
#include "ui/widgets/colorcapturewidget.h"
#include "ui/widgets/bingowidget.h"
#include "ui/widgets/multigamewidget.h"

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
    void onCreateMultiGameRequested(const QList<QColor> &colors);

private:
    void setupMainScreen();
    void updateCenterWidgetPosition();
    bool event(QEvent *event) override;
    QPixmap createBearImage();
    
    QStackedWidget *stackedWidget;
    QWidget *mainMenu;
    QWidget *centerWidget = nullptr;
    ColorCaptureWidget *colorCaptureWidget;
    BingoWidget *bingoWidget;
    MultiGameWidget *multiGameWidget;
};

#endif // MAINWINDOW_H
