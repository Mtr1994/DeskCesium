#include "softconfig.h"

#include <QTextCodec>
#include <QFile>
#include <QMessageBox>

SoftConfig::SoftConfig(QObject *parent) : QObject(parent)
{

}

bool SoftConfig::init()
{
    if (!QFile::exists("conf.ini"))
    {
        QMessageBox::information(nullptr, "消息", "未找到 conf.ini 配置文件", "　　　　　　知道了　　　　　　");
        return false;
    }

    mSetting = new QSettings("conf.ini", QSettings::IniFormat);
    mSetting->setIniCodec(QTextCodec::codecForName("utf-8"));

    return true;
}

SoftConfig *SoftConfig::getInstance()
{
    static SoftConfig config;
    return &config;
}

QString SoftConfig::getValue(const QString& entry, const QString& item)
{
    if (mSetting->isWritable())
    {

    }
    QString value = mSetting->value(entry + "/" + item).toString();
    return value;
}

void SoftConfig::setValue(const QString& pEntry, const QString& pItem, const QString& pValue)
{
    mSetting->setValue(pEntry + "/" + pItem, pValue);
}
