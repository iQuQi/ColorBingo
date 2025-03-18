#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QStackedWidget>
#include <QList>
#include <QColor>
#include <QPixmap>
#include <QPushButton>
#include "background.h"
#include "ui/widgets/colorcapturewidget.h"
#include "ui/widgets/bingowidget.h"
#include "ui/widgets/multigamewidget.h"
#include "matchingwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    P2PNetwork *network;

private slots:
    void showMatchingScreen();
    void showBingoScreen();
    void showMultiGameScreen();
    void showMainMenu();
    void onCreateBingoRequested(const QList<QColor> &colors);
    void onCreateMultiGameRequested(const QList<QColor> &colors);
    void onVolumeButtonClicked();

private:
    bool isLocalMultiGameReady = false;
    bool isOpponentMultiGameReady = false;
    bool isMultiGameStarted = false;
    QList<QColor> storedColors;

    void checkIfBothPlayersReady();
    void onOpponentMultiGameReady();
    void setupMainScreen();
    void updateCenterWidgetPosition();
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    QPixmap createBearImage();
    QPixmap createVolumeImage(int volumeLevel);
    void updateVolumeButton();
    void updateVolumeButtonPosition();
    
    QStackedWidget *stackedWidget;
    QWidget *mainMenu;
    QWidget *centerWidget = nullptr;
    ColorCaptureWidget *colorCaptureWidget;
    BingoWidget *bingoWidget;
    MultiGameWidget *multiGameWidget;
    MatchingWidget *matchingWidget;

    QLabel *waitingLabel;

    
    QPushButton *volumeButton;
    int volumeLevel;
    
    QPixmap backgroundImage;
};

#endif // MAINWINDOW_H
