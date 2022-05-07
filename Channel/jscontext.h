#ifndef JSCONTEXT_H
#define JSCONTEXT_H

#include <QObject>

class JsContext : public QObject
{
    Q_OBJECT
public:
    explicit JsContext(QObject *parent = nullptr);

signals:
    void sgl_send_client_msg(const QString &action, const QString &type, const QString &arg);

public:
    // sent msg to html
    void sendMsg();

public slots:
    void recvMsg(const QString &action, const QString &type, bool status, const QString &arg);

};

#endif // JSCONTEXT_H
