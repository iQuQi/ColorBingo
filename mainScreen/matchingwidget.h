#ifndef MATCHINGWIDGET_H
#define MATCHINGWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QResizeEvent>
#include "p2pnetwork.h"
#include "ui/widgets/bingowidget.h"
#include "utils/pixelartgenerator.h"

class MatchingWidget : public QWidget {
    Q_OBJECT

public:
    explicit MatchingWidget(QWidget *parent = nullptr);
    ~MatchingWidget();

    P2PNetwork *p2p;

    void startMatching();
    void restartMatching();
    void resizeEvent(QResizeEvent *event) override;

signals:
    void switchToBingoScreen();
    void backToMainRequested(); // Back 버튼을 눌렀을 때 메인 화면으로 전환하는 시그널

private slots:
    void updateMatchStatus(QString peerIP);
    void onBackButtonClicked(); // Back button click handler

private:
    QLabel *statusLabel;
    QLabel *bearLeftLabel;
    QLabel *bearRightLabel;
    QVBoxLayout *layout;
    QHBoxLayout *statusLayout;
    QPushButton *backButton; // back button
};

#endif // MATCHINGWIDGET_H


