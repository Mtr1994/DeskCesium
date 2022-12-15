#include "jscontext.h"
#include "Public/appsignal.h"
#include "Public/softconfig.h"

// test
#include <QDebug>

JsContext::JsContext(QObject *parent)
    : QObject{parent}
{

}

void JsContext::sendMsg()
{

}

void JsContext::recvMsg(const QString &action, const QString &type, bool status, const QString &arg, const QString &list)
{
    qDebug() << action << " " << type << " " << status << " " << arg << " " << list.length();
    if (!status)
    {
        return;
    }
    if (action == "add")
    {
        emit AppSignal::getInstance()->sgl_add_entity_finish(type, arg, list);
    }
    else if (action == "delete")
    {
        emit AppSignal::getInstance()->sgl_delete_entity_finish(arg);
    }
    else if (action == "init")
    {
        emit AppSignal::getInstance()->sgl_cesium_init_finish();
    }
}

void JsContext::searchPosition(const QString &longitude, const QString &latitude)
{
    emit AppSignal::getInstance()->sgl_search_local_altitude(longitude.toDouble(), latitude.toDouble());
}
