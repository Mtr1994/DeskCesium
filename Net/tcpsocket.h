#ifndef TCPSOCKET_H
#define TCPSOCKET_H
#include "HPSocket/HPSocket.h"
#include "basesocket.h"

#include <QObject>
#include <QTcpSocket>

// test
#include <QDebug>

class TcpClientListener : public QObject, public ITcpClientListener
{
    Q_OBJECT
public:
    explicit TcpClientListener(QObject *parent = nullptr) : QObject(parent) {};

    EnHandleResult OnPrepareConnect(ITcpClient* pSender, CONNID dwConnID, SOCKET socket)
    {
        Q_UNUSED(pSender);Q_UNUSED(dwConnID);Q_UNUSED(socket);
        return HR_OK;
    }

    EnHandleResult OnConnect(ITcpClient* pSender, CONNID dwConnID)
    {
        Q_UNUSED(pSender);
        emit sgl_tcp_socket_connect(dwConnID);
        return HR_OK;
    }

    EnHandleResult OnHandShake(ITcpClient* pSender, CONNID dwConnID)
    {
        Q_UNUSED(pSender);Q_UNUSED(dwConnID);
        return HR_OK;
    }

    EnHandleResult OnReceive(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
    {
        Q_UNUSED(pSender);
        QByteArray data = QByteArray::fromRawData((const char*)pData, iLength);
        emit sgl_thread_recv_socket_data(dwConnID, data.toStdString());
        return HR_OK;
    }

    EnHandleResult OnReceive(ITcpClient* pSender, CONNID dwConnID, int iLength)
    {
        Q_UNUSED(pSender);Q_UNUSED(dwConnID);Q_UNUSED(iLength);
        return HR_OK;
    }

    EnHandleResult OnSend(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
    {
        Q_UNUSED(pSender);Q_UNUSED(dwConnID);Q_UNUSED(pData);Q_UNUSED(iLength);
        return HR_OK;
    }

    EnHandleResult OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
    {
        Q_UNUSED(pSender);Q_UNUSED(enOperation);Q_UNUSED(iErrorCode);
        emit sgl_tcp_socket_disconnect(dwConnID);
        return HR_OK;
    }

signals:
    void sgl_tcp_socket_connect(uint64_t dwconnid);
    void sgl_tcp_socket_disconnect(uint64_t dwconnid);
    void sgl_thread_recv_socket_data(uint64_t dwconnid, const std::string &data);

};

class TcpSocket : public BaseSocket
{
    Q_OBJECT
public:
    explicit TcpSocket();

    bool connect(const QString &ipv4, uint16_t port);
    bool connect();
    bool write(const std::string &data);
    bool closeSocket();

signals:
    void sgl_tcp_socket_connect(uint64_t dwconnid);
    void sgl_tcp_socket_disconnect(uint64_t dwconnid);
    void sgl_recv_socket_data(uint64_t dwconnid, const std::string &data);

private:
    QString mServerAddress;
    uint16_t mServerPort;

    TcpClientListener mTcpClientListener;
    CTcpClientPtr mTcpClientPtr;
};

#endif // TCPSOCKET_H
