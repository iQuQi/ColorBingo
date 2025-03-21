#include "p2pnetwork.h"
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QDebug>

P2PNetwork *P2PNetwork::instance = nullptr;

// ✅ Singleton 인스턴스를 가져오는 함수
P2PNetwork *P2PNetwork::getInstance() {
    if (!instance) {
        instance = new P2PNetwork();  // ✅ 최초 1회만 생성됨
        qDebug() << "DEBUG: 🔄 P2PNetwork instance has been created!";
    }
    return instance;
}

// ✅ Singleton 인스턴스를 삭제하는 함수
void P2PNetwork::resetInstance() {
    if (instance) {
        P2PNetwork* temp = instance;
        instance = nullptr;  // ✅ 먼저 `nullptr`로 설정
        delete temp;  // ✅ 이제 안전하게 삭제

        qDebug() << "DEBUG: 🔄 P2PNetwork instance reset!";
    }
}

P2PNetwork::P2PNetwork(QObject *parent) : QObject(parent), isMatched(false), isMatchingActive(false), isServerMode(false) {
    qDebug() << "DEBUG: P2PNetwork constructor started";
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(45454, QUdpSocket::ShareAddress);
    connect(udpSocket, &QUdpSocket::readyRead, this, &P2PNetwork::processPendingDatagrams);

    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &P2PNetwork::onNewConnection);
    server->listen(QHostAddress::Any, 50000);

    clientSocket = new QTcpSocket(this);
    connect(clientSocket, &QTcpSocket::connected, this, &P2PNetwork::onClientConnected);
    connect(clientSocket, &QTcpSocket::readyRead, this, &P2PNetwork::onDataReceived);
    connect(clientSocket, &QTcpSocket::disconnected, this, &P2PNetwork::onDisconnected);
    connect(clientSocket, &QTcpSocket::errorOccurred, this, &P2PNetwork::onSocketError);

    matchTimer = new QTimer(this);
    connect(matchTimer, &QTimer::timeout, this, &P2PNetwork::sendMatchRequest);
}

// ✅ 랜덤 매칭 시작
void P2PNetwork::startMatching() {
//    if (isMatchingActive) {
//        qDebug() << "DEBUG: Matching is already active. Ignoring startMatching() call.";
//        return;
//    }

    qDebug() << "DEBUG: Starting new matching process";
    isMatchingActive = false;
    isMatched = false;

    discoveredBoards.clear();
    if (udpSocket->state() != QUdpSocket::BoundState) {
        qDebug() << "WARNING: UDP socket is not bound! Attempting to rebind...";
        udpSocket->bind(45454, QUdpSocket::ShareAddress);
        qDebug() << "DEBUG: ✅ UDP socket successfully rebound";
    }

    if (!server->isListening()) {
        server->listen(QHostAddress::Any, 50000);
    }

    // ✅ 매칭 요청 타이머가 실행 중인지 확인하고, 실행되지 않으면 강제 실행
    if (!matchTimer->isActive()) {
        qDebug() << "DEBUG: matchTimer is not active, starting it now.";
        isMatchingActive = true;
    } else {
        qDebug() << "DEBUG: matchTimer was already running.";
    }

    matchTimer->start(3000);  // 3초마다 매칭 요청 전송
}

// ✅ "매칭 요청"을 같은 네트워크의 모든 보드에게 전송 (UDP 브로드캐스트)
void P2PNetwork::sendMatchRequest() {
    if (!isMatchingActive) {
        qDebug() << "DEBUG: Matching is not active. Skipping match request.";
        return;
    }

    QByteArray message = "MATCH_REQUEST";
    udpSocket->writeDatagram(message, QHostAddress::Broadcast, 45454);
    qDebug() << "DEBUG: 📡 Match request sent via broadcast";
    qDebug() << "DEBUG: isMatched: " << isMatched;
    qDebug() << "DEBUG: isMatchingActive: " << isMatchingActive;

    // ✅ 타이머가 계속 실행 중인지 확인
    if (!matchTimer->isActive()) {
        qDebug() << "WARNING: matchTimer is not running! Restarting now.";
        matchTimer->start(3000);
    }
}

// ✅ UDP 메시지 수신 → "MATCH_RESPONSE"를 보내는 보드 목록 저장
void P2PNetwork::processPendingDatagrams() {
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        QHostAddress sender;
        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender);

        QString senderIP = sender.toString();
        QString localIP = getLocalIPAddress();

        if (!isMatchingActive || senderIP == localIP || isMatched) {
            continue;
        }

        if (datagram == "MATCH_REQUEST") {
            udpSocket->writeDatagram("MATCH_RESPONSE", sender, 45454);
        } else if (datagram == "MATCH_RESPONSE") {
            discoveredBoards.insert(senderIP);
            qDebug() << "DEBUG: 📡 Match response received from:" << senderIP;
        }
    }

    // 랜덤으로 보드 선택
    if (discoveredBoards.size() && !isMatched) {
        QStringList boardList = discoveredBoards.values();

        QString peerIP = boardList[QRandomGenerator::global()->bounded(boardList.size())];

        //emit matchFound(peerIP);
        qDebug() << "DEBUG: 🎯 Match found with:" << peerIP;
        isMatched = true;
        matchTimer->stop();  // 스캔 중지

        clientSocket->connectToHost(peerIP, 50000);  // TCP 연결 시작

    }
}

void P2PNetwork::onNewConnection() {
    connectedClient = server->nextPendingConnection();
    connect(connectedClient, &QTcpSocket::readyRead, this, &P2PNetwork::onDataReceived);
    connect(connectedClient, &QTcpSocket::errorOccurred, this, &P2PNetwork::onSocketError);

    // ✅ 서버 역할: 연결된 상대 보드의 IP 출력
    QString peerIP = connectedClient->peerAddress().toString();
    qDebug() << "DEBUG: 🌐 New client connected! Peer IP:" << peerIP;
    isServerMode = true;
    isMatched = true;
    isMatchingActive = false;
    matchTimer->stop();

    emit matchFound(peerIP);
}

void P2PNetwork::onClientConnected() {
    // ✅ 클라이언트 역할: 연결된 상대 보드의 IP 출력
    QString peerIP = clientSocket->peerAddress().toString();
    qDebug() << "DEBUG: 🔗 Connected to peer! Peer IP:" << peerIP;
    isMatched = true;
    isMatchingActive = false;
    matchTimer->stop();
    // ✅ UI 업데이트
    emit matchFound(peerIP);
    //emit switchToBingoScreen();
}

QString P2PNetwork::getLocalIPAddress() {
    QString localIP;
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol &&
                entry.ip() != QHostAddress::LocalHost) {
                localIP = entry.ip().toString();
                return localIP;
            }
        }
    }
    return "0.0.0.0";
}

void P2PNetwork::disconnectFromPeer() {
    qDebug() << "DEBUG: Disconnecting from peer...";

    isMatched = false;
    isMatchingActive = false;
    discoveredBoards.clear();

    // ✅ 매칭 타이머 중지
    if (matchTimer->isActive()) {
        matchTimer->stop();
        qDebug() << "DEBUG: Match timer stopped.";
    }

    // ✅ TCP 클라이언트 소켓 닫기
    if (!isServerMode) {
        qDebug() << "DEBUG: Closing client socket...";
        clientSocket->abort();  // ✅ 즉시 연결 해제
        clientSocket->close();
    } else {
        qDebug() << "DEBUG: Closing server socket...";
        connectedClient->abort();
        connectedClient->close();
    }

    if (server->isListening()) { // ✅ TCP 서버 종료
        server->close();
    }

    // ✅ UDP 소켓 닫기
    if (udpSocket->state() == QUdpSocket::BoundState) {
        udpSocket->close();
    }

    qDebug() << "DEBUG: ✅ Successfully disconnected!";

}

void P2PNetwork::onDisconnected() {
    qDebug() << "⚠️ WARNING: Connection lost!";
}

void P2PNetwork::onSocketError(QAbstractSocket::SocketError socketError) {
    qDebug() << "ERROR: ❌ Socket error occurred:" << socketError;

    if (socketError == QAbstractSocket::RemoteHostClosedError) {
        qDebug() << "DEBUG: 📡 Opponent disconnected! Declaring victory.";
        emit opponentDisconnected();  // ✅ 상대방이 연결을 끊은 경우
    } else if (socketError == QAbstractSocket::NetworkError ||
               socketError == QAbstractSocket::HostNotFoundError ||
               socketError == QAbstractSocket::ConnectionRefusedError) {
        qDebug() << "DEBUG: ⚠️ Network issue detected! Returning to main page...";
        emit networkErrorOccurred();  // ✅ 내 네트워크 문제
    }
}

void P2PNetwork::sendMultiGameReady() {
    QString message = "CAPTURE_DONE";

    if (isServerMode) {
        connectedClient->write(message.toUtf8() + "\n");
        connectedClient->flush();
        qDebug() << "📤 Sent CAPTURE_DONE message to opponent";

     } else {
        clientSocket->write(message.toUtf8() + "\n");
        clientSocket->flush();
        qDebug() << "📤 Sent CAPTURE_DONE message to opponent";
    }
}

// 상대보드에 점수 전송
void P2PNetwork::sendBingoScore(int score) {

    QString message = QString("SCORE_UPDATE:%1").arg(score);

    if (isServerMode) {
        qDebug() << "DEBUG: " << connectedClient->peerAddress().toString();
        qDebug() << "DEBUG: I'm server!" << connectedClient->state();
        connectedClient->write(message.toUtf8() + "\n");
        connectedClient->flush();
        qDebug() << "DEBUG: 📤 Sent score update to opponent:" << score;
    } else {
        qDebug() << "DEBUG: " << clientSocket->peerAddress().toString();
        qDebug() << "DEBUG: I'm client!" << clientSocket->state();
        clientSocket->write(message.toUtf8() + "\n");
        clientSocket->flush();
        qDebug() << "DEBUG: 📤 Sent score update to opponent:" << score;
    }
}

void P2PNetwork::sendGameOverMessage() {
    QString message = "GAME_OVER";

    if (isServerMode) {
        connectedClient->write(message.toUtf8() + "\n");
        connectedClient->flush();
        qDebug() << "DEBUG: 📤 Sent GAME_OVER message to opponent";
    } else {
        clientSocket->write(message.toUtf8() + "\n");
        clientSocket->flush();
        qDebug() << "DEBUG: 📤 Sent GAME_OVER message to opponent";
    }
}

void P2PNetwork::sendAttackMessage() {
    QString message = "ATTACK";

    if (isServerMode) {
        connectedClient->write(message.toUtf8() + "\n");
        connectedClient->flush();
        qDebug() << "DEBUG: 📤 Sent ATTACK message to opponent";
    } else {
        clientSocket->write(message.toUtf8() + "\n");
        clientSocket->flush();
        qDebug() << "DEBUG: 📤 Sent ATTACK message to opponent";
    }
}

void P2PNetwork::onDataReceived() {
    qDebug() << "onDataReceived has been called";
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;

    QByteArray data = senderSocket->readAll();
    QString message = QString(data).trimmed();
    qDebug() << "DEBUG: 📩 Received message:" << message;

    if (message.startsWith("SCORE_UPDATE:")) {
        int score = message.mid(13).toInt();
        emit opponentScoreUpdated(score);  // 점수 업데이트 시그널 발생
        //qDebug() << "DEBUG: 🔄 Opponent's score updated to:" << score;
    } else if (message == "GAME_OVER") {
        qDebug() << "DEBUG: 🎯 Opponent won! Ending game...";
        emit gameOverReceived();
    } else if (message == "CAPTURE_DONE") {
        qDebug() << "DEBUG: 🎯 Opponent has completed capture!";
        emit opponentMultiGameReady();
    } else if (message == "ATTACK") {
        qDebug() << "DEBUG: Opponent has attacked!";
        emit attackedByOpponent();
    }
}

/*
// ✅ TCP 연결 요청을 받은 보드 (서버 역할)
void P2PNetwork::onNewConnection() {
    connectedClient = server->nextPendingConnection();
    connect(connectedClient, &QTcpSocket::readyRead, this, &P2PNetwork::onDataReceived);
    qDebug() << "🌐 New client connected!";
}

// ✅ TCP 연결이 완료된 클라이언트 (클라이언트 역할)
void P2PNetwork::onClientConnected() {
    qDebug() << "🔗 Connected to peer! Sending MATCH_REQUEST...";
    clientSocket->write("MATCH_REQUEST\n");  // ✅ 매칭 요청 메시지 전송
    clientSocket->flush();
    connectionState = MATCH_REQUEST_SENT;
}

// ✅ 메시지를 수신하면 처리
void P2PNetwork::onDataReceived() {
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;

    QByteArray data = senderSocket->readAll();
    QString message = QString(data).trimmed();

    qDebug() << "📩 Received:" << message;
    MessageType type = parseMessage(message);
    handleMessage(type, senderSocket);
}




// ✅ TCP 연결 요청을 받은 보드 (서버 역할)
void P2PNetwork::onNewConnection() {
    connectedClient = server->nextPendingConnection();
    connect(connectedClient, &QTcpSocket::readyRead, this, &P2PNetwork::onDataReceived);
    qDebug() << "🌐 New client connected!";
}

// ✅ TCP 연결이 완료된 클라이언트 (클라이언트 역할)
void P2PNetwork::onClientConnected() {
    qDebug() << "🔗 Connected to peer!";
    clientSocket->write("Hello from client!\n");
}

// ✅ 메시지를 수신하면 출력
void P2PNetwork::onDataReceived() {
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;

    QByteArray data = senderSocket->readAll();
    QString message = QString(data).trimmed();

    qDebug() << "📩 Received:" << message;
    if (message == "Hello from client!") {
        isMatched = true;
        matchTimer->stop();
        qDebug() << "✅ Match confirmed with peer!";
    }
}
*/


