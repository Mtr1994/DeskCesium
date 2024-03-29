﻿#include "dialogsearch.h"
#include "ui_dialogsearch.h"
#include "Protocol/protocolhelper.h"
#include "Common/common.h"
#include "Control/Message/messagewidget.h"
#include "Public/appsignal.h"
#include "Public/appconfig.h"
#include "Protocol/protocolhelper.h"
#include "Net/usernetworker.h"

// test
#include <QDebug>

DialogSearch::DialogSearch(bool keywordflag, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSearch), mKeyworkSearchFlag(keywordflag)
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

    resize(parentWidget()->width() * 0.72, parentWidget()->height() * 0.6);

    ui->tbParameter->setPlaceholderText(mKeyworkSearchFlag ? "输入关键字" : "输入或选择数据过滤条件");
    ui->tbParameter->setReadOnly(!mKeyworkSearchFlag);
    ui->widgetComplexSearchBase->setVisible(!mKeyworkSearchFlag);

    if (!mKeyworkSearchFlag) ui->widgetSearchParameter->requestSelectParameter();

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
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_DEPTH, new QStandardItem(tr("深度")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_DT_SPEED, new QStandardItem(tr("拖体速度")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_HORIZONTAL_RANGE_DIRECTION, new QStandardItem(tr("水平距离方向")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_FIELD_HORIZONTAL_RANGE_VALUE, new QStandardItem(tr("水平距离值")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_HEIGHT_FROM_BOTTOM, new QStandardItem(tr("离底高度")));

    mModelSideScanSource.setHorizontalHeaderItem(FIELD_R_THETA, new QStandardItem(tr("航迹向分辨率")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_SIDE_SCAN_IMAGE_NAME, new QStandardItem(tr("侧扫图片名称")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_IMAGE_TOP_LEFT_LONGITUDE, new QStandardItem(tr("左上角经度")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_IMAGE_TOP_LEFT_LATITUDE, new QStandardItem(tr("左上角纬度")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_IMAGE_BOTTOM_RIGHT_LONGITUDE, new QStandardItem(tr("右下角经度")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_IMAGE_BOTTOM_RIGHT_LATITUDE, new QStandardItem(tr("右下角纬度")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_IMAGE_TOTAL_BYTE, new QStandardItem(tr("图片字节大小")));

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
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_VERIFY_FLAG, new QStandardItem(tr("查证标志")));
    mModelSideScanSource.setHorizontalHeaderItem(FIELD_STATUS, new QStandardItem(tr("数据状态")));

    // 设置表头格式
    ui->tblvSideScanSource->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->tblvSideScanSource->setModel(&mModelSideScanSource);

    QFont font("Microsoft YaHei", 9);
    QFontMetrics metrics(font);
    ui->tblvSideScanSource->verticalHeader()->setDefaultSectionSize(metrics.height() * 2.4);

    // 隐藏数据项
    ui->tblvSideScanSource->setColumnHidden(FIELD_ID, false);
    ui->tblvSideScanSource->setColumnHidden(FIELD_CRUISE_NUMBER, false);
    ui->tblvSideScanSource->setColumnHidden(FIELD_DIVE_NUMBER, false);
    ui->tblvSideScanSource->setColumnHidden(FIELD_SCAN_LINE, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_CRUISE_YEAR, false);
    ui->tblvSideScanSource->setColumnHidden(FIELD_DT_TIME, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_LONGITUDE, false);
    ui->tblvSideScanSource->setColumnHidden(FIELD_LATITUDE, false);
    ui->tblvSideScanSource->setColumnHidden(FIELD_DEPTH, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_DT_SPEED, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_HORIZONTAL_RANGE_DIRECTION, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_FIELD_HORIZONTAL_RANGE_VALUE, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_HEIGHT_FROM_BOTTOM, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_R_THETA, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_SIDE_SCAN_IMAGE_NAME, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_IMAGE_TOP_LEFT_LONGITUDE, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_IMAGE_TOP_LEFT_LATITUDE, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_IMAGE_BOTTOM_RIGHT_LONGITUDE, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_IMAGE_BOTTOM_RIGHT_LATITUDE, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_IMAGE_TOTAL_BYTE, false);
    ui->tblvSideScanSource->setColumnHidden(FIELD_ALONG_TRACK, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_ACROSS_TRACK, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_REMARKS, false);
    ui->tblvSideScanSource->setColumnHidden(FIELD_SUPPOSE_SIZE, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_PRIORITY, false);
    ui->tblvSideScanSource->setColumnHidden(FIELD_VERIFY_AUV_SSS_IMAGE_PATHS, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_VERIFY_IMAGE_PATHS, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_IMAGE_DESCRIPTION, false);
    ui->tblvSideScanSource->setColumnHidden(FIELD_TARGET_LONGITUDE, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_TARGET_LATITUDE, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_POSITION_ERROR, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_VERIFY_CRUISE_NUMBER, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_VERIFY_DIVE_NUMBER, false);
    ui->tblvSideScanSource->setColumnHidden(FIELD_VERIFY_TIME, true);
    ui->tblvSideScanSource->setColumnHidden(FIELD_VERIFY_FLAG, false);
    ui->tblvSideScanSource->setColumnHidden(FIELD_STATUS, true);

    // 指定列宽
    ui->tblvSideScanSource->setColumnWidth(FIELD_ID, metrics.averageCharWidth() * 36);

    // 信号
    connect(AppSignal::getInstance(), &AppSignal::sgl_remote_entity_add_finish, this, &DialogSearch::slot_remote_entity_add_finish);
    connect(AppSignal::getInstance(), &AppSignal::sgl_query_side_scan_source_data_response, this, &DialogSearch::slot_query_side_scan_source_data_response);
    connect(AppSignal::getInstance(), &AppSignal::sgl_query_trajectory_data_response, this, &DialogSearch::slot_query_trajectory_data_response);
}

void DialogSearch::slot_btn_search_side_scan_click()
{
    QString parameter = ui->tbParameter->text().trimmed();
    if (parameter.isEmpty()) return;

    // 发关键字查询
    if (mKeyworkSearchFlag)
    {
        KeywordSearchParameter searchParameter;
        searchParameter.set_keyword(parameter.toStdString());

        if (!UserNetWorker::getInstance()->getSocketStatus()) return;

        QByteArray pack = ProtocolHelper::getInstance()->createPackage(CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_KEYWORD, searchParameter.SerializeAsString());
        UserNetWorker::getInstance()->sendPack(pack);
    }
    else
    {
        // 发送数据查询命令 （格式化查询命令）
        FilterSearchParameter searchParameter;
        searchParameter.set_cruise_year(QString("\"%1\"").arg(mListCruiseYear.join("\",\"")).toStdString());
        searchParameter.set_cruise_number(QString("\"%1\"").arg(mListCruiseNumber.join("\",\"")).toStdString());
        searchParameter.set_dive_number(QString("\"%1\"").arg(mListDiveNumber.join("\",\"")).toStdString());

        for (auto &item : mListVerifyDiveNumber)
        {
            searchParameter.add_verify_dive_number(item.toStdString());
        }

        if (mListPriority.size() != 3) searchParameter.set_priority(mListPriority.join(",").toStdString());
        if (mListVerifyFlag.size() != 2) searchParameter.set_verify_flag(mListVerifyFlag.join(",").toStdString());

        if (!UserNetWorker::getInstance()->getSocketStatus()) return;

        QByteArray pack = ProtocolHelper::getInstance()->createPackage(CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_FILTER, searchParameter.SerializeAsString());
        UserNetWorker::getInstance()->sendPack(pack);
    }
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
    else if (target == "priority")
    {
        if (append)
        {
            if (!mListPriority.contains(value))
            {
                mListPriority.append(value);
            }
        }
        else
        {
            mListPriority.removeOne(value);
        }
    }
    else if (target == "verify_flag")
    {
        if (append)
        {
            if (!mListVerifyFlag.contains(value))
            {
                mListVerifyFlag.append(value);
            }
        }
        else
        {
            mListVerifyFlag.removeOne(value);
        }
    }


    // 更新检索条件
    QString arg1 = QString("cruise_year:%1").arg(mListCruiseYear.join(","));
    QString arg2 = QString("cruise_number:%1").arg(mListCruiseNumber.join(","));
    QString arg3 = QString("dive_number:%1").arg(mListDiveNumber.join(","));
    QString arg4 = QString("verify_dive_number:%1").arg(mListVerifyDiveNumber.join(","));
    QString arg5 = QString("priority:%1").arg(mListPriority.join(","));
    QString arg6 = QString("verify_flag:%1").arg(mListVerifyFlag.join(","));

    QStringList searchParameterList;
    searchParameterList.append(mListCruiseYear.isEmpty() ? "" : arg1);
    searchParameterList.append(mListCruiseNumber.isEmpty() ? "" : arg2);
    searchParameterList.append(mListDiveNumber.isEmpty() ? "" : arg3);
    searchParameterList.append(mListVerifyDiveNumber.isEmpty() ? "" : arg4);
    searchParameterList.append(mListPriority.isEmpty() ? "" : arg5);
    searchParameterList.append(mListVerifyFlag.isEmpty() ? "" : arg6);
    searchParameterList.removeAll("");

    ui->tbParameter->setText(searchParameterList.join(";"));
}

void DialogSearch::slot_btn_extract_clicked()
{
    if (!ui->tblvSideScanSource->selectionModel()->hasSelection())
    {
        MessageWidget *msg = new MessageWidget(MessageWidget::M_Info, MessageWidget::P_Bottom_Center, this);
        msg->showMessage("请选择要提取的数据");
        mBufferArray.clear();
        return;
    }

    QModelIndexList listIndex = ui->tblvSideScanSource->selectionModel()->selectedRows();

    // 缓存本次已经请求过的轨迹
    QStringList listTrajectory;

    for (auto &index : listIndex)
    {
        QStringList listItemName = {"id", "cruise_number", "dive_number", "scan_line", "cruise_year", "dt_time", "longitude", "latitude", "depth", "dt_speed", "horizontal_range_direction", "horizontal_range_value", "height_from_bottom", "r_theta", "side_scan_image_name", "image_top_left_longitude", "image_top_left_latitude", "image_bottom_right_longitude", "image_bottom_right_latitude", "image_total_byte", "along_track", "across_track", "remarks", "suppose_size", "priority", "verify_auv_sss_image_paths", "verify_image_paths", "image_description", "target_longitude", "target_latitude", "position_error", "verify_cruise_number", "verify_dive_number", "verify_time", "verify_flag"};
        QStringList listValues;
        QString cruiseNumber;
        QString diveNumber;
        QString remotePath;
        uint16_t itemSize = listItemName.size();
        QString ip = AppConfig::getInstance()->getValue("Remote", "ip");
        for (int i = 0; i < itemSize; i++)
        {
            QStandardItem *item = mModelSideScanSource.item(index.row(), i);
            if (nullptr == item) return;

            if (i == FIELD_CRUISE_NUMBER) cruiseNumber = item->text();
            if (i == FIELD_DIVE_NUMBER) diveNumber = item->text();

            if (i == FIELD_SIDE_SCAN_IMAGE_NAME)
            {
                if (!item->text().trimmed().isEmpty()) remotePath = QString("http://%1/image/upload/%2/image/%3").arg(ip, cruiseNumber, item->text().trimmed());
                listValues.append(listItemName.at(i) + ": \"" + remotePath + "\"");
            }
            else if (i == FIELD_VERIFY_AUV_SSS_IMAGE_PATHS)
            {
                QStringList listResult;
                auto list = item->text().split(";", Qt::SkipEmptyParts);
                for (auto &path : list)
                {
                    listResult.append(QString("http://%1/image/upload/%2/image/%3").arg(ip, cruiseNumber, path));
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
                    listResult.append(QString("http://%1/image/upload/%2/image/%3").arg(ip, cruiseNumber, path));
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

        // 循环查询轨迹
        QString trajectoryID = QString("%1-%2").arg(cruiseNumber, diveNumber);
        if (listTrajectory.contains(trajectoryID)) continue;
        listTrajectory.append(trajectoryID);

        RequestTrajectory request;
        request.set_cruise_number(cruiseNumber.toStdString());
        request.set_dive_number(diveNumber.toStdString());

        if (diveNumber.contains("DT"))
        {
            request.set_trajectory_type("DT");
        }
        else if (diveNumber.contains("FDZ") || diveNumber.contains("SY"))
        {
            request.set_trajectory_type("HOV");
        }
        else if (diveNumber.contains("HS"))
        {
            request.set_trajectory_type("AUV");
        }
        else if (diveNumber.contains("TanSuoYiHao") || diveNumber.contains("TanSuoErHao"))
        {
            request.set_trajectory_type("SHIP");
        }

        if (!UserNetWorker::getInstance()->getSocketStatus()) return;

        QByteArray pack = ProtocolHelper::getInstance()->createPackage(CMD_QUERY_TRAJECTORY_BY_CURSE_AND_DIVE, request.SerializeAsString());
        UserNetWorker::getInstance()->sendPack(pack);
    }

    qDebug() << "listTrajectory " << listTrajectory;
}

void DialogSearch::slot_remote_entity_add_finish(const QString &id, bool status, const QString &message)
{
    if (!status)
    {
        MessageWidget *msg = new MessageWidget(MessageWidget::M_Error, MessageWidget::P_Bottom_Center, this);
        msg->showMessage(QString("提取数据 %1 失败，%2").arg(id, message));
        return;
    }

    QModelIndexList listIndex = mModelSideScanSource.match(mModelSideScanSource.index(0, 0), Qt::DisplayRole, id, 1, Qt::MatchExactly | Qt::MatchRecursive);
    if (listIndex.isEmpty()) return;

    mModelSideScanSource.removeRows(listIndex.first().row(), 1);
}

void DialogSearch::slot_query_side_scan_source_data_response(const QList<QStringList> &list)
{
    // 清理旧的数据
    mModelSideScanSource.removeRows(0, mModelSideScanSource.rowCount());

    int size = list.size();
    if (0 == size)
    {
        MessageWidget *msg = new MessageWidget(MessageWidget::M_Info, MessageWidget::P_Bottom_Center, this);
        msg->showMessage("查询结果为空");
        mBufferArray.clear();
        return;
    }

    for(int i = 0; i < size; i++)
    {
        QStringList listItemText = list.at(i);
        QList<QStandardItem*> listItem;
        for (auto &value : listItemText)
        {
            listItem.append(new QStandardItem(value));
        }
        mModelSideScanSource.appendRow(listItem);
    }

    MessageWidget *msg = new MessageWidget(MessageWidget::M_Success, MessageWidget::P_Bottom_Center, this);
    msg->showMessage(QString("检索到 %1 条数据").arg(QString::number(size)));

    ui->lbStatisticsNumber->setText(QString("总计： %1 条记录").arg(QString::number(size)));
}

void DialogSearch::slot_query_trajectory_data_response(const RequestTrajectoryResponse &response)
{
    qDebug() << "slot_query_trajectory_data_response " << response.status();
    if (!response.status()) return;
    qDebug() << "list " << response.position_chains().length();
    emit AppSignal::getInstance()->sgl_add_remote_trajectory_entity(response.id().data(), response.position_chains().data());
}

