#ifndef MATCHINGWIDGET_H
#define MATCHINGWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include "p2pnetwork.h"
#include "ui/widgets/bingowidget.h"

class MatchingWidget : public QWidget {
    Q_OBJECT

public:
    explicit MatchingWidget(QWidget *parent = nullptr);
    ~MatchingWidget();

    P2PNetwork *p2p;

    void startMatching();
    void restartMatching();

signals:
    void switchToBingoScreen();

private slots:
    void updateMatchStatus(QString peerIP);

private:
    QLabel *statusLabel;
    QVBoxLayout *layout;
};

#endif // MATCHINGWIDGET_H


