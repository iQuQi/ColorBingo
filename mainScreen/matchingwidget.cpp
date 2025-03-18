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

    // Back 버튼 생성 및 스타일 설정
    backButton = new QPushButton("Back", this);
    backButton->setFixedSize(100, 30);
    backButton->setStyleSheet(
        "QPushButton { background-color: red; color: white; font-size: 18px; font-weight: bold; border-radius: 8px; }"
        "QPushButton:hover { background-color: darkred; }"
    );

//    layout->addWidget(backButton, 0, Qt::AlignRight | Qt::AlignBottom); // 🔹 우측 하단 배치
//    setLayout(layout);

    // ✅ 매칭이 완료되면 updateMatchStatus() 실행
    connect(p2p, &P2PNetwork::matchFound, this, &MatchingWidget::updateMatchStatus);

    // Back 버튼 클릭 시 동작
    connect(backButton, &QPushButton::clicked, this, &MatchingWidget::onBackButtonClicked);
}

// ✅ 화면 크기가 변경될 때 back 버튼을 우측 하단에 위치하도록 설정
void MatchingWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    // 🔹 우측 하단 여백 (20px)
    int marginX = 20;
    int marginY = 20;

    // 🔹 버튼 위치 계산 (화면 크기 - 버튼 크기 - 여백)
    int x = width() - backButton->width() - marginX;
    int y = height() - backButton->height() - marginY;

    // 🔹 back 버튼 이동
    backButton->move(x, y);
}

MatchingWidget::~MatchingWidget() {
    qDebug() << "DEBUG: MatchingWidget Destructor Called";

    // ✅ P2P 네트워크에서 안전하게 매칭 중단
    P2PNetwork::getInstance()->disconnectFromPeer();
//    delete p2p;
}

void MatchingWidget::startMatching() {
    statusLabel->setText("Matching...");
    p2p->isMatchingActive = true;
    p2p->isMatched = false;
    p2p->startMatching();
}

// Back 버튼 클릭 시 동작
void MatchingWidget::onBackButtonClicked() {
    qDebug() << "DEBUG: Back button clicked in MatchingWidget";

    if (!p2p->isMatched) {  // 매칭이 이루어지지 않았다면
        qDebug() << "DEBUG: Stopping matching process and returning to main menu";
        p2p->disconnectFromPeer();  // 매칭을 중단
        emit backToMainRequested(); // 메인 화면으로 돌아가도록 시그널 발생
    } else {
        qDebug() << "DEBUG: Match already completed, cannot go back!";
    }
}

// ✅ 매칭이 완료되면 `MultiGameWidget`으로 이동
void MatchingWidget::updateMatchStatus(QString peerIP) {
    statusLabel->setText("Matched with " + peerIP + "!");
    qDebug() << "DEBUG: Matched with" << peerIP << ", switching to Bingo screen in 3 seconds...";
    p2p->isMatchingActive = false;

    // ✅ `MultiGameWidget` 실행
    QTimer::singleShot(3000, this, [=]() {
        qDebug() << "DEBUG: Swtching to multi game screen";
        emit switchToBingoScreen();  // ✅ MainWindow에서 Bingo 화면으로 전환
        // ✅ 현재 `MatchingWidget` 닫기
        this->close();
    });
}
