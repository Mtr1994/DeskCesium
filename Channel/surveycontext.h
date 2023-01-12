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

    // 添加异常点实体
    void sgl_add_remote_point_entitys(const QString &array);

    // 添加轨迹线实体
    void sgl_add_remote_trajectory_entitys(const QString &obj);

public slots:
    void recvWebMsg(const QString &action, bool status);

};

#endif // SURVEYCONTEXT_H
