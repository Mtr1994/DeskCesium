#include "usernetworker.h"
#include "Net/tcpsocket.h"
#include "Public/appconfig.h"
#include "Common/common.h"
#include "Protocol/protocolhelper.h"
#include "Public/appsignal.h"

UserNetWorker::UserNetWorker(QObject *parent)
    : QObject{parent}
{

}

UserNetWorker *UserNetWorker::getInstance()
{
    static UserNetWorker worker;
    return &worker;
}

void UserNetWorker::init()
{
    // 准备执行 SQL 任务
    mTcpSocket = new TcpSocket;
    connect(mTcpSocket, &TcpSocket::sgl_recv_socket_data, this, &UserNetWorker::slot_recv_tcp_socket_data);
    connect(mTcpSocket, &TcpSocket::sgl_tcp_socket_connect, this, &UserNetWorker::slot_tcp_socket_connect);
    connect(mTcpSocket, &TcpSocket::sgl_tcp_socket_disconnect, this, &UserNetWorker::slot_tcp_socket_disconnect);

    QString ip = AppConfig::getInstance()->getValue("Remote", "ip");
    mTcpSocket->connect(ip, 60011);
}

void UserNetWorker::sendPack(const QByteArray &pack)
{
    if ((nullptr == mTcpSocket) || (!mTcpSocketConnected) || (pack.length() == 0)) return;
    mTcpSocket->write(pack.toStdString());
}

void UserNetWorker::slot_tcp_socket_connect(uint64_t dwconnid)
{
    Q_UNUSED(dwconnid);
    mTcpSocketConnected = true;

    emit AppSignal::getInstance()->sgl_tcp_socket_status_change(true);
}

void UserNetWorker::slot_tcp_socket_disconnect(uint64_t dwconnid)
{
    Q_UNUSED(dwconnid);
    mTcpSocketConnected = false;

    emit AppSignal::getInstance()->sgl_tcp_socket_status_change(false);
}

void UserNetWorker::slot_recv_tcp_socket_data(uint64_t dwconnid, const std::string &data)
{
    Q_UNUSED(dwconnid);
    ProtocolHelper::getInstance()->parsePackage(QByteArray::fromStdString(data));
}
