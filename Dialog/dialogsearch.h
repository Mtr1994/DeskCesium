﻿#ifndef DIALOGSEARCH_H
#define DIALOGSEARCH_H

#include <QDialog>
#include <QStandardItemModel>

namespace Ui {
class DialogSearch;
}

class TcpSocket;
class DialogSearch : public QDialog
{
    Q_OBJECT

public:
    enum {FIELD_ID = 0, FIELD_CRUISE_NUMBER, FIELD_DIVE_NUMBER, FIELD_SCAN_LINE, FIELD_CRUISE_YEAR, FIELD_DT_TIME,
          FIELD_LONGITUDE, FIELD_LATITUDE, FIELD_DT_SPEED, FIELD_HORIZONTAL_RANGE_DIRECTION, FIELD_FIELD_HORIZONTAL_RANGE_VALUE,
          FIELD_HEIGHT_FROM_BOTTOM, FIELD_R_THETA, FIELD_SIDE_SCAN_IMAGE_NAME, FIELD_IMAGE_TOP_LEFT_LONGITUDE, FIELD_IMAGE_TOP_LEFT_LATITUDE,
          FIELD_IMAGE_BOTTOM_RIGHT_LONGITUDE, FIELD_IMAGE_BOTTOM_RIGHT_LATITUDE, FILD_IMAGE_TOTAL_BYTE, FIELD_ALONG_TRACK, FIELD_ACROSS_TRACK, FIELD_REMARKS,
          FIELD_SUPPOSE_SIZE, FIELD_PRIORITY, FIELD_VERIFY_AUV_SSS_IMAGE_PATHS, FIELD_VERIFY_IMAGE_PATHS, FIELD_IMAGE_DESCRIPTION, FIELD_TARGET_LONGITUDE,
          FIELD_TARGET_LATITUDE, FIELD_POSITION_ERROR, FIELD_VERIFY_CRUISE_NUMBER, FIELD_VERIFY_DIVE_NUMBER, FIELD_VERIFY_TIME, FIELD_VERIFY_FLAG, FIELD_STATUS };

    explicit DialogSearch(const QString &parameter, QWidget *parent = nullptr);
    ~DialogSearch();

private:
    void init();

private slots:
    void slot_tcp_socket_connect(uint64_t dwconnid);

    void slot_recv_socket_data(uint64_t dwconnid, const std::string &data);

    void slot_tcp_socket_disconnect(uint64_t dwconnid);

    void slot_btn_search_side_scan_click();

    void slot_modify_search_parameter(const QString &target, const QString &value, bool append);

    void slot_btn_extract_clicked();

private:
    Ui::DialogSearch *ui;

    QString mParameter;

    // 网络通信服务
    TcpSocket *mTcpSocket = nullptr;
    bool mTcpSocketConnected = false;

    QStringList mListCruiseYear;
    QStringList mListCruiseNumber;
    QStringList mListDiveNumber;
    QStringList mListVerifyDiveNumber;
    QStringList mListPriority;
    QStringList mListVerifyFlag;

    QStandardItemModel mModelSideScanSource;

    // 数据通信缓存
    QByteArray mBufferArray;
};

#endif // DIALOGSEARCH_H
