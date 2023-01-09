#include "chartcontext.h"

ChartContext::ChartContext(QObject *parent)
    : QObject{parent}
{

}

void ChartContext::recvWebMsg(const QString &action, bool status)
{
    if (action == "init" && status)
    {
        emit sgl_web_view_init_finish();
    }
}
