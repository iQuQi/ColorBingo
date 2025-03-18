#include "matchingwidget.h"

MatchingWidget::MatchingWidget(QWidget *parent)
    : QWidget(parent)
    //, p2p(new P2PNetwork(this))
{
    //bingoWidget = new BingoWidget(this);
    p2p = P2PNetwork::getInstance();
    statusLabel = new QLabel("Waiting for match...", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #333;");

    bearLeftLabel = new QLabel(this);
    bearRightLabel = new QLabel(this);
    QPixmap bearPixmap = PixelArtGenerator::getInstance()->createBearImage();
    bearPixmap = bearPixmap.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    bearLeftLabel->setPixmap(bearPixmap);
    bearRightLabel->setPixmap(bearPixmap);

    statusLayout = new QHBoxLayout();
    statusLayout->setAlignment(Qt::AlignCenter);
    statusLayout->addWidget(bearLeftLabel);
    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(bearRightLabel);

    layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignCenter);
    layout->addLayout(statusLayout);
    setLayout(layout);

    // ✅ 매칭이 완료되면 updateMatchStatus() 실행
    connect(p2p, &P2PNetwork::matchFound, this, &MatchingWidget::updateMatchStatus);
}

MatchingWidget::~MatchingWidget() {
    delete p2p;
}

void MatchingWidget::startMatching() {
    statusLabel->setText("Matching...");
    p2p->isMatchingActive = true;
    p2p->isMatched = false;
    p2p->startMatching();
}

// ✅ 매칭이 완료되면 `MultiGameWidget`으로 이동
void MatchingWidget::updateMatchStatus(QString peerIP) {
    statusLabel->setText("🎯 Matched with " + peerIP + "!");
    qDebug() << "DEBUG: 🎉 Matched with" << peerIP << ", switching to Bingo screen in 3 seconds...";
    p2p->isMatchingActive = false;

    // ✅ `MultiGameWidget` 실행
    QTimer::singleShot(3000, this, [=]() {
        qDebug() << "DEBUG: Swtching to multi game screen";
        emit switchToBingoScreen();  // ✅ MainWindow에서 Bingo 화면으로 전환
        // ✅ 현재 `MatchingWidget` 닫기
        this->close();
    });
}
