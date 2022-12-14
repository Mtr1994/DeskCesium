#ifndef APPSIGNAL_H
#define APPSIGNAL_H

#include <QObject>

class QWidget;
class AppSignal : public QObject
{
    Q_OBJECT
private:
    explicit AppSignal(QObject *parent = nullptr);
    AppSignal(const AppSignal& signal) = delete;
    AppSignal& operator=(const AppSignal& signal) = delete;

public:
    static AppSignal* getInstance();

signals:
    // 添加 KML 实体
    void sgl_add_kml_entity(const QString &path);

    // 添加 KMZ 实体
    void sgl_add_kmz_entity(const QString &path);

    // 添加 TIFF 实体
    void sgl_add_tiff_entity(const QString &path);

    // 添加远程 TIFF 实体
    void sgl_add_remote_tiff_entity(const QString &path, const QString &remoteobject);

    // 添加 grd 实体
    void sgl_add_grd_entity(const QString &path);

    // 添加实体结果
    void sgl_add_entity_finish(const QString &type, const QString &arg, const QString &list);

    // 删除实体结果
    void sgl_delete_entity_finish(const QString &arg);

    // 改变实体展示状态
    void sgl_change_entity_status(const QString &type, const QString &name, bool visible, const QString &parentid = "");

    // 飞到选中实体
    void sgl_fly_to_entity(const QString &type, const QString &id, const QString &parentId);

    // 根据 名称 删除实体
    void sgl_delete_cesium_data_source(const QString &type, const QString &name);

    // 修改鼠标浮动拾取功能开启状态
    void sgl_change_mouse_over_pick(bool open);

    // 检索本地高程数据
    void sgl_search_local_altitude(double longitude, double latitude);

    // 系统错误信息报告 (线程)
    void sgl_thread_report_system_error(const QString & msg);

    // 系统错误信息报告
    void sgl_report_system_error(const QString & msg);

    // cesium 加载完成
    void sgl_cesium_init_finish();
};

#endif // APPSIGNAL_H
