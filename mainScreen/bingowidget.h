#ifndef BINGOWIDGET_H
#define BINGOWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QRandomGenerator>

class BingoWidget : public QWidget {
    Q_OBJECT

public:
    explicit BingoWidget(QWidget *parent = nullptr);
    void generateRandomColors();

private:
    QGridLayout *gridLayout;
    QLabel *bingoCells[3][3];
};

#endif // BINGOWIDGET_H
