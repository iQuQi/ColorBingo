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
    explicit P2PNetwork(QObject *parent = nullptr);

    void startMatching();
    void disconnectFromPeer();

    bool isMatchingActive;
    bool isMatched;

signals:
    void matchFound(QString peerIP);
    void switchToBingoScreen();

private slots:
    void processPendingDatagrams();
    void onNewConnection();
    void onClientConnected();
    //void onDataReceived();
    void sendMatchRequest();


private:
    QString getLocalIPAddress();

    QUdpSocket *udpSocket;
    QTcpServer *server;
    QTcpSocket *clientSocket;
    QTcpSocket *connectedClient;
    QSet<QString> discoveredBoards;
    QTimer *matchTimer;
};

#endif // P2PNETWORK_H


