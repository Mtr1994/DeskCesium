#include "appsignal.h"
#include <qmetatype.h>

AppSignal::AppSignal(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<int64_t>("int64_t");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<std::string>("std::string");
}

AppSignal *AppSignal::getInstance()
{
    static AppSignal appSignal;
    return &appSignal;
}
