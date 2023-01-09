#ifndef CHARTCONTEXT_H
#define CHARTCONTEXT_H

#include <QObject>

class ChartContext : public QObject
{
    Q_OBJECT
public:
    explicit ChartContext(QObject *parent = nullptr);

signals:
    // 发送组件初始化完毕信息
    void sgl_web_view_init_finish();

    // 加载 年份数据 折线图
    void sgl_load_year_curve_chart(const QString &dataObject);

    // 加载 优先级 饼图
    void sgl_load_priority_pie_chart(const QString &dataObject);

    // 加载 查证 饼图
    void sgl_load_check_pie_chart(const QString &dataObject);

public slots:
    void recvWebMsg(const QString &action, bool status);

};

#endif // CHARTCONTEXT_H
