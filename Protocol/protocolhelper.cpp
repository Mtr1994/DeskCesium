#include "protocolhelper.h"
#include "Common/common.h"
#include "Public/appsignal.h"

#include <QFile>
#include <QDateTime>
#include <thread>
#include <QDir>
#include <fstream>
#include <QTextCodec>
#include <regex>

using namespace std;

// test
#include <QDebug>

ProtocolHelper::ProtocolHelper(QObject *parent)
    : QObject{parent}
{

}

ProtocolHelper *ProtocolHelper::getInstance()
{
    static ProtocolHelper protocolHelper;
    return &protocolHelper;
}

void ProtocolHelper::init()
{
    // 开启一个数据包解析线程
    auto func = std::bind(&ProtocolHelper::parse, this);
    std::thread th(func);
    th.detach();
}

QByteArray ProtocolHelper::createPackage(uint32_t cmd, const std::string &para)
{
    uint32_t size = para.length();
    QByteArray array;
    array.resize(size + 8);

    uint16_t version = 0x1107;
    // 版本
    memcpy(array.data(), &version, 2);

    // 命令
    memcpy(array.data() + 2, &cmd, 2);

    // 长度
    memcpy(array.data() + 4, &size, 4);

    // 数据
    memcpy(array.data() + 8, para.data(), size);
    return array;
}

void ProtocolHelper::parsePackage(const QString &channel, const QByteArray &data)
{
    if (mDataChannel != channel)
    {
        mBufferArray.clear();
        mDataChannel = channel;
    }

    std::unique_lock<std::mutex> lock(mBufferMutex);
    mBufferArray.append(data);

    mCvPackParse.notify_one();
}

void ProtocolHelper::parse()
{
    std::mutex mutexParse;
    while (mParsePackage)
    {
        std::unique_lock<std::mutex> lockParse(mutexParse);
        mCvPackParse.wait(lockParse, [this]{ return mBufferArray.size() >= 8;});

        std::unique_lock<std::mutex> lockBuffer(mBufferMutex);



        lockBuffer.unlock();


        switch (0) {
        case CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA_RESPONSE:
        {
            qDebug() << QString("程序未处理的命令 %1").arg(QString::number(CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA_RESPONSE));
            break;
        }
        default:
            break;
        }

        std::lock_guard<std::mutex> lastLock(mBufferMutex);
        mBufferArray = mBufferArray.right(mBufferArray.size() - 0);
    }
}
