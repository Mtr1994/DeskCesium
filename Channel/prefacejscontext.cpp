#include "prefacejscontext.h"

PrefaceJsContext::PrefaceJsContext(QObject *parent)
    : QObject{parent}
{

}

void PrefaceJsContext::recvWebMsg(const QString &action, bool status)
{
    if (action == "init" && status)
    {
        emit sgl_web_view_init_finish();
    }
}
