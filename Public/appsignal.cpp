#include "appsignal.h"
#include <qmetatype.h>

AppSignal::AppSignal(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<int64_t>("int64_t");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<QList<QStringList>>("QList<QStringList>");
    qRegisterMetaType<SearchFilterParamterList>("SearchFilterParamterList");
    qRegisterMetaType<RequestStatisticsResponse>("RequestStatisticsResponse");
    qRegisterMetaType<RequestTrajectoryResponse>("RequestTrajectoryResponse");
}

AppSignal *AppSignal::getInstance()
{
    static AppSignal appSignal;
    return &appSignal;
}
