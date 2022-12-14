#ifndef WIDGETSELECTPARAMETER_H
#define WIDGETSELECTPARAMETER_H

#include <QWidget>

namespace Ui {
class WidgetSelectParameter;
}

class TcpSocket;
class WidgetSelectParameter : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetSelectParameter(QWidget *parent = nullptr);
    ~WidgetSelectParameter();

signals:
    // 修改检索条件
    void sgl_modify_search_parameter(const QString &target, const QString &value, bool append);

private:
    void init();

private slots:
    void slot_tcp_socket_connect(uint64_t dwconnid);

    void slot_recv_socket_data(uint64_t dwconnid, const std::string &data);

    void slot_tcp_socket_disconnect(uint64_t dwconnid);

private:
    Ui::WidgetSelectParameter *ui;

    // 网络通信服务
    TcpSocket *mTcpSocket = nullptr;
    bool mTcpSocketConnected = false;
};

#endif // WIDGETSELECTPARAMETER_H
