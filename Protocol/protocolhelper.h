#ifndef PROTOCOLHELPER_H
#define PROTOCOLHELPER_H

#include <QObject>
#include <string>
#include <QByteArray>
#include <QString>
#include <mutex>

class ProtocolHelper : public QObject
{
    Q_OBJECT
private:
    explicit ProtocolHelper(QObject *parent = nullptr);
    ProtocolHelper(const ProtocolHelper& signal) = delete;
    ProtocolHelper& operator=(const ProtocolHelper& signal) = delete;

public:
    static ProtocolHelper* getInstance();

    void init();

    QByteArray createPackage(uint32_t cmd, const std::string &para = "");

    void parsePackage(const QByteArray &data);

private:
    void parse();

private:
    QByteArray mBufferArray;

    std::mutex mBufferMutex;
    std::condition_variable mCvPackParse;

    bool mParsePackage = false;

    // 缓存状态消息
    QString mStatusMessage;

    int64_t mCurrentPackSize = 0;
};

#endif // PROTOCOLHELPER_H
