#ifndef BINGOPREPARATIONWIDGET_H
#define BINGOPREPARATIONWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QThread>
#include <QMessageBox>
#include <QTimer>
#include "hardwareInterface/v4l2camera.h"
#include "p2pnetwork.h"

// 게임 모드 열거형 추가
enum class GameMode {
    SINGLE,
    MULTI
};

class BingoPreparationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BingoPreparationWidget(QWidget *parent = nullptr);
    ~BingoPreparationWidget();

    void stopCameraCapture();
    
    // 게임 모드 설정 메서드 추가
    void setGameMode(GameMode mode);
    GameMode getGameMode() const { return gameMode; }

signals:
    void createBingoRequested(const QList<QColor> &colors);
    void createMultiGameRequested(const QList<QColor> &colors);
    void backToMainRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onCreateBingoClicked();
    void onCreateMultiGameClicked();
    void onBackButtonClicked();
    void updateCameraFrame();
    void handleCameraDisconnect();
    void onOpponentDisconnected();

private:
    QLabel *cameraView;
    QWidget *buttonPanel;
    V4L2Camera *camera;
    bool isCapturing;
    GameMode gameMode; // 현재 게임 모드 저장 변수 추가

    void startCamera();
    QList<QColor> captureColorsFromFrame();

    // 네트워크
    P2PNetwork *network;
};

#endif // BINGOPREPARATIONWIDGET_H 
