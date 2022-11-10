#include "tcpsocket.h"

#include <QHostAddress>

// test
#include <QDebug>

TcpSocket::TcpSocket()
{

}

bool TcpSocket::connect(const QString &ipv4, uint16_t port)
{
    mServerAddress = ipv4;
    mServerPort = port;
    mTcpClientPtr.Attach(TcpClient_Creator::Create(&mTcpClientListener));

    QObject::connect(&mTcpClientListener, &TcpClientListener::sgl_tcp_socket_connect, this, &TcpSocket::sgl_tcp_socket_connect);
    QObject::connect(&mTcpClientListener, &TcpClientListener::sgl_tcp_socket_disconnect, this, &TcpSocket::sgl_tcp_socket_disconnect);
    QObject::connect(&mTcpClientListener, &TcpClientListener::sgl_thread_recv_socket_data, this, &TcpSocket::sgl_recv_socket_data, Qt::QueuedConnection);

    return mTcpClientPtr->Start(LPCTSTR(ipv4.toStdWString().data()), port);
}

bool TcpSocket::connect()
{
    return mTcpClientPtr->Start(LPCTSTR(mServerAddress.toStdWString().data()), mServerPort);
}

QString TcpSocket::getServerKey()
{
    return QString("TCPCLIENT:%1:%2").arg(mServerAddress, QString::number(mServerPort));
}

bool TcpSocket::write(const std::string &data)
{
    return mTcpClientPtr->Send((BYTE*)data.data(), data.length());
}

bool TcpSocket::closeSocket()
{
    return mTcpClientPtr->Stop();
}
