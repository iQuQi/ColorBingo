#include "p2pnetwork.h"
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QDebug>

P2PNetwork::P2PNetwork(QObject *parent) : QObject(parent), isMatched(false), isMatchingActive(false) {
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(45454, QUdpSocket::ShareAddress);
    connect(udpSocket, &QUdpSocket::readyRead, this, &P2PNetwork::processPendingDatagrams);

    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &P2PNetwork::onNewConnection);
    server->listen(QHostAddress::Any, 50000);

    clientSocket = new QTcpSocket(this);
    connect(clientSocket, &QTcpSocket::connected, this, &P2PNetwork::onClientConnected);
    //connect(clientSocket, &QTcpSocket::readyRead, this, &P2PNetwork::onDataReceived);

    matchTimer = new QTimer(this);
    connect(matchTimer, &QTimer::timeout, this, &P2PNetwork::sendMatchRequest);
}

// ✅ 랜덤 매칭 시작
void P2PNetwork::startMatching() {
    discoveredBoards.clear();
    matchTimer->start(3000);  // 3초마다 매칭 요청 전송
}

// ✅ "매칭 요청"을 같은 네트워크의 모든 보드에게 전송 (UDP 브로드캐스트)
void P2PNetwork::sendMatchRequest() {
    QByteArray message = "MATCH_REQUEST";
    udpSocket->writeDatagram(message, QHostAddress::Broadcast, 45454);
    qDebug() << "📡 Match request sent via broadcast";
    qDebug() << "isMatched: " << isMatched;
    qDebug() << "isMatchingActive: " << isMatchingActive;
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
            qDebug() << "📡 Match response received from:" << senderIP;
        }
    }

    // 랜덤으로 보드 선택
    if (discoveredBoards.size() && !isMatched) {
        QStringList boardList = discoveredBoards.values();

        QString peerIP = boardList[QRandomGenerator::global()->bounded(boardList.size())];

        //emit matchFound(peerIP);
        qDebug() << "🎯 Match found with:" << peerIP;
        isMatched = true;
        matchTimer->stop();  // 스캔 중지

        clientSocket->connectToHost(peerIP, 50000);  // TCP 연결 시작

    }
}

void P2PNetwork::onNewConnection() {
    connectedClient = server->nextPendingConnection();

    // ✅ 서버 역할: 연결된 상대 보드의 IP 출력
    QString peerIP = connectedClient->peerAddress().toString();
    qDebug() << "🌐 New client connected! Peer IP:" << peerIP;
    isMatched = true;
    isMatchingActive = false;
    matchTimer->stop();
    qDebug() << "emit matchFound";
    // ✅ UI 업데이트
    emit matchFound(peerIP);
    qDebug() << "after emit matchFound";
    //emit switchToBingoScreen();
}

void P2PNetwork::onClientConnected() {
    // ✅ 클라이언트 역할: 연결된 상대 보드의 IP 출력
    QString peerIP = clientSocket->peerAddress().toString();
    qDebug() << "🔗 Connected to peer! Peer IP:" << peerIP;
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
    qDebug() << "🔌 Disconnecting from peer...";

    isMatched = false;
    isMatchingActive = false;
    discoveredBoards.clear();

    // ✅ TCP 클라이언트 소켓 닫기
    if (clientSocket->state() == QAbstractSocket::ConnectedState) {
        clientSocket->disconnectFromHost();
        if (clientSocket->state() != QAbstractSocket::UnconnectedState) {
            clientSocket->waitForDisconnected(1000);
        }
    }

    // ✅ TCP 서버 종료
    if (server->isListening()) {
        server->close();
    }

    // ✅ UDP 소켓 닫기
    if (udpSocket->state() == QUdpSocket::BoundState) {
        udpSocket->close();
    }

    // ✅ 매칭 상태 초기화
    isMatched = false;
    discoveredBoards.clear();

    qDebug() << "✅ Successfully disconnected!";
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


