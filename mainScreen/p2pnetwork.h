#ifndef P2PNETWORK_H
#define P2PNETWORK_H

#include <QObject>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSet>
#include <QTimer>

class P2PNetwork : public QObject {
    Q_OBJECT
public:

    static P2PNetwork *getInstance();
    static void resetInstance();
    void startMatching();
    void disconnectFromPeer();
    void sendBingoScore(int score);
    void sendGameOverMessage();
    void sendMultiGameReady();

    bool isMatchingActive;
    bool isMatched;

signals:
    void matchFound(QString peerIP);
    void switchToBingoScreen();
    void opponentScoreUpdated(int score);
    void gameOverReceived();
    void opponentMultiGameReady();
    void opponentDisconnected();
    void networkErrorOccurred();

private slots:
    void processPendingDatagrams();
    void onNewConnection();
    void onClientConnected();
    void onDataReceived();
    void sendMatchRequest();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    explicit P2PNetwork(QObject *parent = nullptr);
    static P2PNetwork *instance;

    bool isServerMode;
    QString getLocalIPAddress();

    QUdpSocket *udpSocket;
    QTcpServer *server;
    QTcpSocket *clientSocket;
    QTcpSocket *connectedClient;
    QSet<QString> discoveredBoards;
    QTimer *matchTimer;
};

#endif // P2PNETWORK_H


