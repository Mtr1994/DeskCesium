#ifndef SOFTCONFIG_H
#define SOFTCONFIG_H

#include <QObject>
#include <QSettings>

class SoftConfig : public QObject
{
    Q_OBJECT
public:
    static SoftConfig *getInstance();

    bool init();

    QString getValue(const QString &entry, const QString &item);

    void setValue(const QString &pEntry, const QString &pItem, const QString &pValue);

private:
    explicit SoftConfig(QObject *parent = nullptr);


private:
    QSettings* mSetting;
};

#endif // SOFTCONFIG_H
