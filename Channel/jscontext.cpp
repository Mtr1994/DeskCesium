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

void JsContext::recvMsg(const QString &action, const QString &type, bool status, const QString &arg, const QString &list)
{
    qDebug() << action << " " << type << " " << status << " " << arg << " " << list.length();
    if (!status)
    {
        // 如果添加失败，提示用户
        if (type.contains("remote point") || type.contains("remote tif"))
        {
            emit AppSignal::getInstance()->sgl_remote_entity_add_finish(arg, false, list);
        }
        return;
    }
    if (action == "add")
    {
        emit AppSignal::getInstance()->sgl_add_entity_finish(type, arg, list);

        // 如果添加成功，检索框表格要删除该条记录，表示成功提取
        if (type.contains("remote point") || type.contains("remote tif"))
        {
            emit AppSignal::getInstance()->sgl_remote_entity_add_finish(arg, true, "成功");
        }
    }
    else if (action == "delete")
    {
        emit AppSignal::getInstance()->sgl_delete_entity_finish(arg);
    }
    else if (action == "init")
    {
        emit sgl_web_view_init_finish();
    }
}

void JsContext::searchPosition(const QString &longitude, const QString &latitude)
{
    emit AppSignal::getInstance()->sgl_search_local_altitude(longitude.toDouble(), latitude.toDouble());
}
