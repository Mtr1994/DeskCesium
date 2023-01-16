#ifndef APPSIGNAL_H
#define APPSIGNAL_H

#include "Proto/sidescansource.pb.h"

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

    // 添加远程 Trajectory 实体
    void sgl_add_remote_trajectory_entity(const QString &id, const QString &positionchains);

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

    // remote entity add finish
    void sgl_remote_entity_add_finish(const QString &id, bool status, const QString &message);

    // TCP 网络状态变化
    void sgl_tcp_socket_status_change(bool status);

    // 文件服务器状态查询结果
    void sgl_ftp_server_work_status(bool status, const QString &message);

    // 异常点数据录入结果
    void sgl_insert_side_scan_source_data_response(bool status, const QString &message);

    // 轨迹信息录入结果
    void sgl_insert_cruise_route_source_data_response(bool status, const QString &message);

    // 异常点数据查询结果
    void sgl_query_side_scan_source_data_response(const QList<QStringList> &list);

    // 轨迹线数据查询结果
    void sgl_query_trajectory_data_response(const RequestTrajectoryResponse &response);

    // 查询检索条件结果
    void sgl_query_search_filter_parameter_response(const SearchFilterParamterList &response);

    // 查询统计数据结果
    void sgl_query_statistics_data_by_condition_response(const RequestStatisticsResponse &response);
};

#endif // APPSIGNAL_H
