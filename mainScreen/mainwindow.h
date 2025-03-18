#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QStackedWidget>
#include <QList>
#include <QColor>
#include <QPixmap>
#include <QPushButton>
#include <QLabel>
#include "background.h"
#include "ui/widgets/bingopreparationwidget.h"
#include "ui/widgets/bingowidget.h"
#include "ui/widgets/multigamewidget.h"
#include "matchingwidget.h"
#include "utils/pixelartgenerator.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    P2PNetwork *network;

private slots:
    void showMatchingScreen();
    void showBingoScreen();
    void showMultiGameScreen();
    void showMainMenu();
    void onCreateBingoRequested(const QList<QColor> &colors);
    void onCreateMultiGameRequested(const QList<QColor> &colors);

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;


private:
    bool isLocalMultiGameReady = false;
    bool isOpponentMultiGameReady = false;
    bool isMultiGameStarted = false;
    QList<QColor> storedColors;

    void checkIfBothPlayersReady();
    void onOpponentMultiGameReady();
    void setupMainScreen();
    void updateCenterWidgetPosition();
    
    // 볼륨 버튼 관련 함수
    void onVolumeButtonClicked();
    void updateVolumeButton();
    void updateVolumeButtonPosition();
    
    QStackedWidget *stackedWidget;
    QWidget *mainMenu;
    QWidget *centerWidget;
    
    BingoPreparationWidget *colorCaptureWidget;
    BingoWidget *bingoWidget;
    MultiGameWidget *multiGameWidget;
    MatchingWidget *matchingWidget;


    QLabel *waitingLabel;


    
    // 볼륨 버튼 및 관련 변수
    QPushButton *volumeButton;
    int volumeLevel; // 볼륨 레벨 (1: 낮음, 2: 중간, 3: 높음)
    
    // 배경 이미지
    QPixmap backgroundImage;

};

#endif // MAINWINDOW_H
