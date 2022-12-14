#include "dialogsearch.h"
#include "ui_dialogsearch.h"
#include "Net/tcpsocket.h"
#include "Proto/sidescansource.pb.h"
#include "Protocol/protocolhelper.h"
#include "Common/common.h"
#include "Control/Message/messagewidget.h"
#include "Public/appsignal.h"

// test
#include <QDebug>

DialogSearch::DialogSearch(const QString &parameter, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSearch), mParameter(parameter)
{
    ui->setupUi(this);

    init();
}

DialogSearch::~DialogSearch()
{
    delete ui;
}

void DialogSearch::init()
{
    setWindowTitle("数据检索");
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

    connect(ui->widgetSearchParameter, &WidgetSelectParameter::sgl_modify_search_parameter, this, &DialogSearch::slot_modify_search_parameter);

    // 准备执行 SQL 任务
    mTcpSocket = new TcpSocket;
    connect(mTcpSocket, &TcpSocket::sgl_recv_socket_data, this, &DialogSearch::slot_recv_socket_data);
    connect(mTcpSocket, &TcpSocket::sgl_tcp_socket_connect, this, &DialogSearch::slot_tcp_socket_connect);
    connect(mTcpSocket, &TcpSocket::sgl_tcp_socket_disconnect, this, &DialogSearch::slot_tcp_socket_disconnect);
    mTcpSocket->connect("101.34.253.220", 60011);

    connect(ui->btnSearchSideScan, &QPushButton::clicked, this, &DialogSearch::slot_btn_search_side_scan_click);
    connect(ui->btnExtract, &QPushButton::clicked, this, &DialogSearch::slot_btn_extract_clicked);

    // 设置表头

    mModelSideScanSource.setHorizontalHeaderItem(FIELD_ID, new QStandardItem(tr("编号")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_CRUISE_NUMBER, new QStandardItem(tr("航次号")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_DIVE_NUMBER, new QStandardItem(tr("潜次号")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_SCAN_LINE, new QStandardItem(tr("侧线号")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_CRUISE_YEAR, new QStandardItem(tr("年份")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_DT_TIME, new QStandardItem(tr("拖体记录时间")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_LONGITUDE, new QStandardItem(tr("经度")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_LATITUDE, new QStandardItem(tr("纬度")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_DT_SPEED, new QStandardItem(tr("拖体速度")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_HORIZONTAL_RANGE_DIRECTION, new QStandardItem(tr("水平距离方向")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_FIELD_HORIZONTAL_RANGE_VALUE, new QStandardItem(tr("水平距离值")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_HEIGHT_FROM_BOTTOM, new QStandardItem(tr("离底高度")));

    mModelSideScanSource.setHorizontalHeaderItem(FIELD_R_THETA, new QStandardItem(tr("航迹向分辨率")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_SIDE_SCAN_IMAGE_NAME, new QStandardItem(tr("侧扫图片名称")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_ALONG_TRACK, new QStandardItem(tr("长")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_ACROSS_TRACK, new QStandardItem(tr("宽")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_REMARKS, new QStandardItem(tr("备注说明")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_SUPPOSE_SIZE, new QStandardItem(tr("推测尺寸")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_PRIORITY, new QStandardItem(tr("优先级")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_VERIFY_AUV_SSS_IMAGE_PATHS, new QStandardItem(tr("AUV 侧扫查证图片路径")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_VERIFY_IMAGE_PATHS, new QStandardItem(tr("查证图片路径")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_IMAGE_DESCRIPTION, new QStandardItem(tr("查证照片描述")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_TARGET_LONGITUDE, new QStandardItem(tr("目标经度")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_TARGET_LATITUDE, new QStandardItem(tr("目标纬度")));

    mModelSideScanSource.setHorizontalHeaderItem(FIELD_POSITION_ERROR, new QStandardItem(tr("定位误差")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_VERIFY_CRUISE_NUMBER, new QStandardItem(tr("查证航次号")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_VERIFY_DIVE_NUMBER, new QStandardItem(tr("查证潜次号")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_VERIFY_TIME, new QStandardItem(tr("查证时间")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_STATUS, new QStandardItem(tr("数据状态")));

    // 隐藏数据项
    ui->tblvSideScanSource->setColumnHidden(FIELD_STATUS, true);

    // 设置表头格式
    ui->tblvSideScanSource->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->tblvSideScanSource->setModel(&mModelSideScanSource);
}

void DialogSearch::slot_tcp_socket_connect(uint64_t dwconnid)
{
    Q_UNUSED(dwconnid);
    mTcpSocketConnected = true;

    if (mParameter.isEmpty()) return;
    if (nullptr == mTcpSocket) return;
}

void DialogSearch::slot_recv_socket_data(uint64_t dwconnid, const std::string &data)
{
    Q_UNUSED(dwconnid);

    mBufferArray.append(QByteArray::fromStdString(data));

    uint32_t cmd = 0;
    memcpy(&cmd, mBufferArray.data() + 2, 2);

    int64_t size = 0;
    memcpy(&size, mBufferArray.data() + 4, 4);

    if (size >  mBufferArray.size()) return;

    switch (cmd) {
    case CMD_QUERY_SIDE_SCAN_SOURCE_DATA_RESPONSE:
    {
        SideScanSourceList response;
        bool status = response.ParseFromString(mBufferArray.mid(8, size).toStdString());
        if (!status)
        {
            qDebug() << "数据解析错误，请联系管理员";
            mBufferArray.clear();
            return;
        }

        int size = response.list_size();

        if (0 == size)
        {
            MessageWidget *msg = new MessageWidget(MessageWidget::M_Info, MessageWidget::P_Bottom_Center, this);
            msg->showMessage("查询结果为空");
            mBufferArray.clear();
            return;
        }

        for(int i = 0; i < size; i++)
        {
            QList<QStandardItem*> listItem;
            listItem.append(new QStandardItem(response.list().at(i).id().data()));
            listItem.append(new QStandardItem(response.list().at(i).cruise_number().data()));
            listItem.append(new QStandardItem(response.list().at(i).dive_number().data()));
            listItem.append(new QStandardItem(response.list().at(i).scan_line().data()));
            listItem.append(new QStandardItem(response.list().at(i).cruise_year().data()));
            listItem.append(new QStandardItem(response.list().at(i).dt_time().data()));
            listItem.append(new QStandardItem(QString::number(response.list().at(i).longitude(), 'f', 6)));
            listItem.append(new QStandardItem(QString::number(response.list().at(i).latitude(), 'f', 6)));
            listItem.append(new QStandardItem(QString::number(response.list().at(i).dt_speed(), 'f', 6)));
            listItem.append(new QStandardItem(response.list().at(i).horizontal_range_direction().data()));
            listItem.append(new QStandardItem(response.list().at(i).horizontal_range_value().data()));
            listItem.append(new QStandardItem(QString::number(response.list().at(i).height_from_bottom(), 'f', 6)));
            listItem.append(new QStandardItem(QString::number(response.list().at(i).r_theta(), 'f', 6)));
            listItem.append(new QStandardItem(response.list().at(i).side_scan_image_name().data()));
            listItem.append(new QStandardItem(QString::number(response.list().at(i).along_track(), 'f', 6)));
            listItem.append(new QStandardItem(QString::number(response.list().at(i).across_track(), 'f', 6)));
            listItem.append(new QStandardItem(response.list().at(i).remarks().data()));
            listItem.append(new QStandardItem(response.list().at(i).suppose_size().data()));
            listItem.append(new QStandardItem(QString::number(response.list().at(i).priority())));
            listItem.append(new QStandardItem(response.list().at(i).verify_auv_sss_image_paths().data()));
            listItem.append(new QStandardItem(response.list().at(i).verify_image_paths().data()));
            listItem.append(new QStandardItem(response.list().at(i).image_description().data()));
            listItem.append(new QStandardItem(response.list().at(i).target_longitude().data()));
            listItem.append(new QStandardItem(response.list().at(i).target_latitude().data()));
            listItem.append(new QStandardItem(response.list().at(i).position_error().data()));
            listItem.append(new QStandardItem(response.list().at(i).verify_cruise_number().data()));
            listItem.append(new QStandardItem(response.list().at(i).verify_dive_number().data()));
            listItem.append(new QStandardItem(response.list().at(i).verify_time().data()));
            listItem.append(new QStandardItem(QString::number(response.list().at(i).status_flag())));

            mModelSideScanSource.appendRow(listItem);
        }

        break;
    }
    default:
        break;
    }

    mBufferArray.clear();
}

void DialogSearch::slot_tcp_socket_disconnect(uint64_t dwconnid)
{
    Q_UNUSED(dwconnid);
    mTcpSocketConnected = false;
}

void DialogSearch::slot_btn_search_side_scan_click()
{
    //QString parameter = ui->tbParameter->text().trimmed();

    // 发送数据查询命令 （格式化查询命令）
    SearchParameter searchParameter;
    searchParameter.set_cruise_year(QString("\"%1\"").arg(mListCruiseYear.join("\",\"")).toStdString());
    searchParameter.set_cruise_number(QString("\"%1\"").arg(mListCruiseNumber.join("\",\"")).toStdString());
    searchParameter.set_dive_number(QString("\"%1\"").arg(mListDiveNumber.join("\",\"")).toStdString());
    searchParameter.set_verify_dive_number(QString("\"%1\"").arg(mListVerifyDiveNumber.join("\",\"")).toStdString());

    if (nullptr == mTcpSocket) return;

    QByteArray pack = ProtocolHelper::getInstance()->createPackage(CMD_QUERY_SIDE_SCAN_SOURCE_DATA, searchParameter.SerializeAsString());
    mTcpSocket->write(pack.toStdString());

    // 顺带就清理旧的数据
    mModelSideScanSource.removeRows(0, mModelSideScanSource.rowCount());
}

void DialogSearch::slot_modify_search_parameter(const QString &target, const QString &value, bool append)
{
    if (target == "cruise_year")
    {
        if (append)
        {
            if (!mListCruiseYear.contains(value))
            {
                mListCruiseYear.append(value);
            }
        }
        else
        {
            mListCruiseYear.removeOne(value);
        }
    }
    else if (target == "cruise_number")
    {
        if (append)
        {
            if (!mListCruiseNumber.contains(value))
            {
                mListCruiseNumber.append(value);
            }
        }
        else
        {
            mListCruiseNumber.removeOne(value);
        }
    }
    else if (target == "dive_number")
    {
        if (append)
        {
            if (!mListDiveNumber.contains(value))
            {
                mListDiveNumber.append(value);
            }
        }
        else
        {
            mListDiveNumber.removeOne(value);
        }
    }
    else if (target == "verify_dive_number")
    {
        if (append)
        {
            if (!mListVerifyDiveNumber.contains(value))
            {
                mListVerifyDiveNumber.append(value);
            }
        }
        else
        {
            mListVerifyDiveNumber.removeOne(value);
        }
    }
    // 更新检索条件
    QString arg1 = QString("cruise_year:%1").arg(mListCruiseYear.join(","));
    QString arg2 = QString("cruise_number:%1").arg(mListCruiseNumber.join(","));
    QString arg3 = QString("dive_number:%1").arg(mListDiveNumber.join(","));
    QString arg4 = QString("verify_dive_number:%1").arg(mListVerifyDiveNumber.join(","));

    QStringList searchParameterList;
    searchParameterList.append(mListCruiseYear.isEmpty() ? "" : arg1);
    searchParameterList.append(mListCruiseNumber.isEmpty() ? "" : arg2);
    searchParameterList.append(mListDiveNumber.isEmpty() ? "" : arg3);
    searchParameterList.append(mListVerifyDiveNumber.isEmpty() ? "" : arg4);
    searchParameterList.removeAll("");

    ui->tbParameter->setText(searchParameterList.join(";"));
}

void DialogSearch::slot_btn_extract_clicked()
{
    if (!ui->tblvSideScanSource->selectionModel()->hasSelection())
    {
        return;
    }

    QModelIndex index = ui->tblvSideScanSource->currentIndex();

//    id = 1;
//    cruise_number = 2;
//    dive_number = 3;
//    scan_line = 4;
//    cruise_year = 5;
//    dt_time = 6;
//    longitude = 7;
//    latitude = 8;
//    dt_speed = 9;
//    horizontal_range_direction = 10;
//    horizontal_range_value = 11;
//    height_from_bottom = 12;
//    r_theta = 13;
//    side_scan_image_name = 14;
//    along_track = 15;
//    across_track = 16;
//    remarks = 17;
//    suppose_size = 18;
//    priority = 19;
//    verify_auv_sss_image_paths = 20;
//    verify_image_paths = 21;
//    image_description = 22;
//    target_longitude = 23;
//    target_latitude = 24;
//    position_error = 25;
//    verify_cruise_number = 26;
//    verify_dive_number = 27;
//    verify_time = 28;

    QStringList listItemName = {"id", "cruise_number", "dive_number", "scan_line", "cruise_year", "dt_time", "longitude", "latitude", "dt_speed", "horizontal_range_direction", "horizontal_range_value", "height_from_bottom", "r_theta", "side_scan_image_name", "along_track", "across_track", "remarks", "suppose_size", "priority", "verify_auv_sss_image_paths", "verify_image_paths", "image_description", "target_longitude", "target_latitude", "position_error", "verify_cruise_number", "verify_dive_number", "verify_time"};
    QStringList listValues;
    QString cruiseNumber;
    QString remotePath;
    for (int i = 0; i < 28; i++)
    {
        QStandardItem *item = mModelSideScanSource.item(index.row(), i);
        if (nullptr == item) return;

        if (i == FIELD_CRUISE_NUMBER) cruiseNumber = item->text();
        if (i == FIELD_SIDE_SCAN_IMAGE_NAME)
        {
            if (!item->text().trimmed().isEmpty()) remotePath = QString("http://101.34.253.220/image/upload/%1/image/%2").arg(cruiseNumber, item->text().trimmed());
        }

        if (i == FIELD_VERIFY_AUV_SSS_IMAGE_PATHS)
        {
            QStringList listResult;
            auto list = item->text().split(";", Qt::SkipEmptyParts);
            for (auto &path : list)
            {
                listResult.append(QString("http://101.34.253.220/image/upload/%1/image/%2").arg(cruiseNumber, path));
            }
            if (listResult.size() > 0) listValues.append(listItemName.at(i) + ": [\"" + listResult.join("\",\"") + "\"]");
            else listValues.append(listItemName.at(i) + ": []");
        }
        else if (i == FIELD_VERIFY_IMAGE_PATHS)
        {
            QStringList listResult;
            auto list = item->text().split(";", Qt::SkipEmptyParts);
            for (auto &path : list)
            {
                listResult.append(QString("http://101.34.253.220/image/upload/%1/image/%2").arg(cruiseNumber, path));
            }
            if (listResult.size() > 0) listValues.append(listItemName.at(i) + ": [\"" + listResult.join("\",\"") + "\"]");
            else listValues.append(listItemName.at(i) + ": []");
        }
        else
        {
            listValues.append(listItemName.at(i) + ": \"" + item->text() + "\"");
        }
    }

    emit AppSignal::getInstance()->sgl_add_remote_tiff_entity(remotePath, QString("{%1}").arg(listValues.join(",").remove("\n")));
}
