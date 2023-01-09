#ifndef SURVEYCONTEXT_H
#define SURVEYCONTEXT_H

#include <QObject>

class SurveyContext : public QObject
{
    Q_OBJECT
public:
    explicit SurveyContext(QObject *parent = nullptr);

signals:
    // 发送组件初始化完毕信息
    void sgl_web_view_init_finish();

public slots:
    void recvWebMsg(const QString &action, bool status);

};

#endif // SURVEYCONTEXT_H
