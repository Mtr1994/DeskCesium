#ifndef USERNETWORKER_H
#define USERNETWORKER_H

#include <QObject>

class TcpSocket;
class UserNetWorker : public QObject
{
    Q_OBJECT
private:
    explicit UserNetWorker(QObject *parent = nullptr);
    UserNetWorker(const UserNetWorker& signal) = delete;
    UserNetWorker& operator=(const UserNetWorker& signal) = delete;

public:
    static UserNetWorker* getInstance();

    void init();

    void sendPack(const QByteArray &pack);

    inline bool getSocketStatus() { return mTcpSocketConnected; };

private slots:
    void slot_tcp_socket_connect(uint64_t dwconnid);
    void slot_tcp_socket_disconnect(uint64_t dwconnid);
    void slot_recv_tcp_socket_data(uint64_t dwconnid, const std::string &data);

private:
    // 网络通信服务
    TcpSocket *mTcpSocket = nullptr;

    // 网络状态
    bool mTcpSocketConnected = false;
};

#endif // USERNETWORKER_H
