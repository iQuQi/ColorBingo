#ifndef COLORSAMPLING_H
#define COLORSAMPLING_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <vector>
#include <QDebug>
#include <QTimer>
#include "v4l2camera.h"

class ColorSamplingWidget : public QWidget {
    Q_OBJECT
    
public:
    ColorSamplingWidget(QWidget *parent = nullptr);
    ~ColorSamplingWidget();
    
    std::vector<QColor> getSampledColors() const;
    
signals:
    void colorSamplingCompleted(const std::vector<QColor>& colors);
    void backButtonClicked();
    
private slots:
    void onGenerateBoardClicked();
    void updateCameraFeed(const QImage& image);
    
private:
    QPushButton *generateBoardButton;
    QPushButton *backButton;
    QLabel *cameraFeedLabel;
    V4L2Camera *camera;
    
    std::vector<QColor> sampledColors;
    
    void extractColorsFromImage(const QImage& image);
    void startCamera();
    void stopCamera();
};

#endif // COLORSAMPLING_H 