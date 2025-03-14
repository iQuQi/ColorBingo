#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include "bingowidget.h"
#include "colorcapturewidget.h"

namespace Ui {
class MainWindow;
}

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

    QWidget *mainScreen;
    QWidget *centerWidget = nullptr; // 중앙 컨테이너 위젯 저장용
    ColorCaptureWidget *colorCaptureWidget;
    BingoWidget *bingoWidget;
    
    void updateCenterWidgetPosition(); // 중앙 위젯 위치 업데이트
    bool eventFilter(QObject *watched, QEvent *event) override; // 이벤트 필터
};

#endif // MAINWINDOW_H
