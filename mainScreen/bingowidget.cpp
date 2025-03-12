#include "bingowidget.h"

BingoWidget::BingoWidget(QWidget *parent) : QWidget(parent) {
    gridLayout = new QGridLayout(this);

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            bingoCells[row][col] = new QLabel(this);
            bingoCells[row][col]->setFixedSize(100, 100);
            bingoCells[row][col]->setAutoFillBackground(true);
            gridLayout->addWidget(bingoCells[row][col], row, col);
        }
    }

    setLayout(gridLayout);
    generateRandomColors();
}

void BingoWidget::generateRandomColors() {
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            int r = QRandomGenerator::global()->bounded(256);
            int g = QRandomGenerator::global()->bounded(256);
            int b = QRandomGenerator::global()->bounded(256);

            QPalette palette = bingoCells[row][col]->palette();
            palette.setColor(QPalette::Window, QColor(r, g, b));
            bingoCells[row][col]->setPalette(palette);
        }
    }
}
