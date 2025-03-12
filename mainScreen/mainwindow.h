#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStackedWidget>
#include "bingowidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void showBingoScreen();
    void showMainMenu();

private:
    QStackedWidget *stackedWidget;
    QWidget *mainMenu;
    BingoWidget *bingoWidget;
};

#endif // MAINWINDOW_H
