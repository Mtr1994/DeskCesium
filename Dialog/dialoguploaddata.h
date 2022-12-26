#ifndef DIALOGUPLOADDATA_H
#define DIALOGUPLOADDATA_H

#include <QDialog>
#include <mutex>
#include <QMultiMap>
#include <QQueue>
#include <QFileInfoList>

#include "Proto/sidescansource.pb.h"

namespace Ui {
class DialogUploadData;
}

typedef struct DataUploadTask
{
    // 任务类型 upload (上传文件) insert （写入数据库）
    QString type;

    std::string arg1;
    std::string arg2 = "";

    uint16_t arg3 = 0;

} DataUploadTask;

class FtpManager;
class TcpSocket;
class DialogUploadData : public QDialog
{
    Q_OBJECT
    enum { STATUS_SUCCESS = 1, STATUS_INFO, STATUS_ERROR };
    enum { CELL_ID = 2, CELL_CRUISE_YEAR, CELL_DT_TIME, CELL_LONGITUDE, CELL_LATITUDE, CELL_DEPTH, CELL_DT_SPEED, CELL_HORIZONTAL_RANGE_DIRECTION, CELL_HORIZONTAL_RANGE_VALUE,
           CELL_HEIGHT_FROM_BOTTOM, CELL_T_THETA, CELL_SIDE_SCAN_IMAGE_NAME, CELL_ALONG_TRACK, CELL_ACROSS_TRACK, CELL_REMARKS, CELL_SUPPOSE_SIZE, CELL_PRIORITY, CELL_AUV_VERIFY_IMAGE_PATHS,
           CELL_VERIFY_IMAGE_PATHS, CELL_IMAGE_DESCRIPTION, CELL_TARGET_LONGITUDE,CELL_TARGET_LATITUDE, CELL_POSITION_ERROR, CELL_VERIFY_CRUISE_NUMBER, CELL_VERIFY_DIVE_NUMBER, CELL_VERIFY_TIME, CELL_VERIFY_FLAG, CELL_STATUS };
public:
    explicit DialogUploadData(QWidget *parent = nullptr);
    ~DialogUploadData();

    void init();

signals:
    void sgl_thread_report_check_status(uint8_t status, const QString &msg, bool stop = true);

    void sgl_send_system_notice_message(uint8_t status, const QString &msg, bool stop = false);

    void sgl_thread_check_data_finish();

    void sgl_start_next_task();

private:
    // 在线程中检索数据的有效性
    void checkData();

    QFileInfoList traverseFolder(const QString &dir);

private slots:
    void slot_btn_upload_data_click();

    void slot_btn_select_dir_click();

    void slot_thread_report_check_status(uint8_t status, const QString &msg, bool stop = true);

    void slot_thread_check_data_finish();

    void slot_recv_socket_data(uint64_t dwconnid, const std::string &data);

    void slot_start_next_task();

private:
    Ui::DialogUploadData *ui;

    // 数据检查线程运行标志
    bool mRunThreadCheck = false;

    // 数据检查检查线程锁
    std::mutex mMutexCheck;

    // 文件服务
    FtpManager *mFtpManager = nullptr;
    bool mFtpServerFlag = false;

    // 网络通信服务
    TcpSocket *mTcpSocket = nullptr;
    bool mTcpServerFlag = false;

    // 任务队列
    QQueue<DataUploadTask> mTaskQueue;
    // 队列线程锁
    std::mutex mMutexQueue;
};

#endif // DIALOGUPLOADDATA_H
