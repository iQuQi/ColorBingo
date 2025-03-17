#ifndef COLORCAPTUREWIDGET_H
#define COLORCAPTUREWIDGET_H

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

class ColorCaptureWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ColorCaptureWidget(QWidget *parent = nullptr);
    ~ColorCaptureWidget();

    void stopCameraCapture();

signals:
    void createBingoRequested(const QList<QColor> &colors);
    void backToMainRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onCreateBingoClicked();
    void onBackButtonClicked();
    void updateCameraFrame();
    void handleCameraDisconnect();

private:
    QLabel *cameraView;
    QWidget *buttonPanel;
    V4L2Camera *camera;
    bool isCapturing;

    void startCamera();
    QList<QColor> captureColorsFromFrame();
};

#endif // COLORCAPTUREWIDGET_H 