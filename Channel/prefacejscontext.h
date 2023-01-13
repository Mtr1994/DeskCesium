#ifndef PREFACEJSCONTEXT_H
#define PREFACEJSCONTEXT_H

#include <QObject>

class PrefaceJsContext : public QObject
{
    Q_OBJECT
public:
    explicit PrefaceJsContext(QObject *parent = nullptr);

signals:
    // 发送组件初始化完毕信息
    void sgl_web_view_init_finish();

    // 添加 preface 信息
    void sgl_change_preface_info(const QString &obj);

public slots:
    void recvWebMsg(const QString &action, bool status);
};

#endif // PREFACEJSCONTEXT_H
