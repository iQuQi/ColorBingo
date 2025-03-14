#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStackedWidget>
#include "bingowidget.h"
#include "multigamewidget.h"

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
    void showMultiGameScreen();
    void showMainMenu();

private:
    QStackedWidget *stackedWidget;
    QWidget *mainMenu;
    BingoWidget *bingoWidget;
    MultiGameWidget *multiGameWidget;
};

#endif // MAINWINDOW_H
