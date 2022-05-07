#include "jscontext.h"
#include "Public/appsignal.h"


// test
#include <QDebug>

JsContext::JsContext(QObject *parent)
    : QObject{parent}
{

}

void JsContext::sendMsg()
{

}

void JsContext::recvMsg(const QString &action, const QString &type, bool status, const QString &arg)
{
    qDebug() << action << " " << status << " " << arg;

    if (action == "add") emit AppSignal::getInstance()->sgl_add_entity_finish(type, arg);
    else if (action == "delete") emit AppSignal::getInstance()->sgl_delete_entity_finish(arg);
}
