#ifndef JSCONTEXT_H
#define JSCONTEXT_H

#include <QObject>

class JsContext : public QObject
{
    Q_OBJECT
public:
    explicit JsContext(QObject *parent = nullptr);

signals:
    // add entity
    void sgl_add_entity(const QString &type, const QString &path);

    // modify  entity visiblity
    void sgl_change_entity_visible(const QString &type, const QString &id, bool visible, const QString &parentid = "");

    // delete entity
    void sgl_delete_entity(const QString &type, const QString &id);

    // start measure
    void sgl_start_measure(const QString &type, const QString &id);

    // start fly to
    void sgl_fly_to_entity(const QString &type, const QString &id, const QString &parentId);

    // change mouse over
    void sgl_change_mouse_over_status(bool open);

    // search mouse altitude
    void sgl_search_mouse_over_altitude(double longitude, double latitude, bool result, double altitude);

    // 发送组件初始化完毕信息
    void sgl_web_view_init_finish();

public:
    // sent msg to html
    void sendMsg();

public slots:
    void recvMsg(const QString &action, const QString &type, bool status, const QString &arg, const QString &list = "");

    void searchPosition(const QString &longitude, const QString &latitude);

};

#endif // JSCONTEXT_H
