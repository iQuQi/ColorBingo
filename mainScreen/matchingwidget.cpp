#include "matchingwidget.h"

MatchingWidget::MatchingWidget(QWidget *parent)
    : QWidget(parent)
    , p2p(new P2PNetwork(this))
{
    bingoWidget = new BingoWidget(this);

    statusLabel = new QLabel("🔄 Waiting for match...", this);
    layout = new QVBoxLayout(this);
    layout->addWidget(statusLabel);
    setLayout(layout);

    // ✅ 매칭이 완료되면 updateMatchStatus() 실행
    connect(p2p, &P2PNetwork::matchFound, this, &MatchingWidget::updateMatchStatus);
}

MatchingWidget::~MatchingWidget() {
    delete p2p;
}

void MatchingWidget::startMatching() {
    statusLabel->setText("🔄 Matching...");
    p2p->isMatchingActive = true;
    p2p->isMatched = false;
    p2p->startMatching();
}

void MatchingWidget::restartMatching() {
    if (p2p) {
        delete p2p;
        p2p = new P2PNetwork(this);
    }
    statusLabel->setText("🔄 Matching...");
    p2p->isMatchingActive = true;
    p2p->isMatched = false;
    p2p->startMatching();
}

// ✅ 매칭이 완료되면 `BingoWidget`으로 이동
void MatchingWidget::updateMatchStatus(QString peerIP) {
    statusLabel->setText("🎯 Matched with " + peerIP + "!");
    qDebug() << "🎉 Matched with" << peerIP << ", switching to Bingo screen in 3 seconds...";
    p2p->isMatchingActive = false;

    // ✅ `BingoWidget` 실행
    QTimer::singleShot(3000, this, [=]() {
        emit switchToBingoScreen();  // ✅ MainWindow에서 Bingo 화면으로 전환
        // ✅ 현재 `MatchingWidget` 닫기
        this->close();
    });
}
