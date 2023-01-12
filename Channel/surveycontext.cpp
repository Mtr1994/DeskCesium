#include "surveycontext.h"

SurveyContext::SurveyContext(QObject *parent)
    : QObject{parent}
{

}

void SurveyContext::recvWebMsg(const QString &action, bool status)
{
    if (action == "init" && status)
    {
        emit sgl_web_view_init_finish();
    }
}
