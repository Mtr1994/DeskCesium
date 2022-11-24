﻿#include "dialoguploaddata.h"
#include "ui_dialoguploaddata.h"
#include "Public/appsignal.h"
#include "xlsxdocument.h"
#include "Ftp/ftpmanager.h"
#include "Net/tcpsocket.h"
#include "Common/common.h"

#include <QDateTime>
#include <thread>
#include <QDir>
#include <QFileDialog>
#include <QStandardPaths>
#include <regex>

// test
#include <QDebug>

// 根据 cell 类别，获取相应的值
QVariant getCellValue(const QXlsx::Cell* cell)
{
    if (nullptr == cell) return "";
    switch (cell->cellType())
    {
    case QXlsx::Cell::DateType:
        return cell->dateTime();
        break;
    default:
        return cell->value();
        break;
    }
}

QByteArray createPackage(uint16_t cmd, const QByteArray &data = "")
{
    uint32_t size = data.length();
    QByteArray array;
    array.resize(size + 8);

    uint16_t version = 0x1107;
    // 版本
    memcpy(array.data(), &version, 2);

    // 命令
    memcpy(array.data() + 2, &cmd, 2);

    // 长度
    memcpy(array.data() + 4, &size, 4);

    // 数据
    memcpy(array.data() + 8, data.data(), size);
    return array;
}

DialogUploadData::DialogUploadData(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogUploadData)
{
    ui->setupUi(this);

    init();
}

DialogUploadData::~DialogUploadData()
{
    mRunThreadCheck = false;

    // 关闭网络链接
    mTcpSocket->closeSocket();
    mTcpSocket->deleteLater();

    delete ui;
}

void DialogUploadData::init()
{
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<std::string>("std::string");

    setWindowTitle("数据上传");
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    resize(parentWidget()->width() * 0.8, parentWidget()->height() * 0.6);

    connect(ui->btnUploadData, &QPushButton::clicked, this, &DialogUploadData::slot_btn_upload_data_click);
    connect(ui->btnSelectDir, &QPushButton::clicked, this, &DialogUploadData::slot_btn_select_dir_click);

    connect(this, &DialogUploadData::sgl_thread_report_check_status, this, &DialogUploadData::slot_thread_report_check_status, Qt::QueuedConnection);
    connect(this, &DialogUploadData::sgl_thread_check_data_finish, this, &DialogUploadData::slot_thread_check_data_finish, Qt::QueuedConnection);
    connect(this, &DialogUploadData::sgl_send_system_notice_message, this, &DialogUploadData::slot_thread_report_check_status);
    connect(this, &DialogUploadData::sgl_start_next_task, this, &DialogUploadData::slot_start_next_task);

    // 准备执行 SQL 任务
    mTcpSocket = new TcpSocket;
    connect(mTcpSocket, &TcpSocket::sgl_recv_socket_data, this, &DialogUploadData::slot_recv_socket_data);
    connect(mTcpSocket, &TcpSocket::sgl_tcp_socket_connect, this, [this](uint64_t dwconnid)
    {
        Q_UNUSED(dwconnid);
        emit sgl_send_system_notice_message(STATUS_SUCCESS, QString("数据服务已连接"));
        mTcpServerFlag = true;

        // 数据服务连接后，通过数据服务查询文件服务是否开启
        QByteArray pack = createPackage(CMD_QUERY_FTP_SERVER_STATUS);
        mTcpSocket->write(pack.toStdString());
    });
    connect(mTcpSocket, &TcpSocket::sgl_tcp_socket_disconnect, this, [this](uint64_t dwconnid)
    {
        Q_UNUSED(dwconnid);
        emit sgl_send_system_notice_message(STATUS_ERROR, QString("数据服务连接失败或断开"));
        mTcpServerFlag = false;
    });

    emit sgl_send_system_notice_message(STATUS_INFO, "尝试连接远程数据服务 ...");
    mTcpSocket->connect("101.34.253.220", 60011);

    // test
    ui->tbRootDir->setText("C:/Users/admin/Desktop/TestExample/TS2-12-1");
}

void DialogUploadData::checkData()
{
    emit sgl_thread_report_check_status(STATUS_INFO, QString(72, '-'), false);
    emit sgl_thread_report_check_status(STATUS_INFO, "开始检查数据有效性", false);

    QDir rootDir = ui->tbRootDir->text();
    if (!rootDir.exists())
    {
        emit sgl_thread_report_check_status(STATUS_ERROR, QString("主文件夹 %1 不存在").arg(rootDir.absolutePath()));
        return;
    }

    // 查找 xlsx 文件是否存在
    auto listXlsx = rootDir.entryInfoList(QStringList("*.xlsx"), QDir::Files | QDir::NoSymLinks);

    // 循环检查 Excel 文件有效性
    for (auto &xlsxFile : listXlsx)
    {
        // 使用工具类查看该文件是否能打开，提示可能存在的中文路路径问题
        QXlsx::Document xlsx(xlsxFile.absoluteFilePath());
        auto listSheetName = xlsx.sheetNames();
        // 查看第一张表是否存在，不存在报告没有找到数据
        if (listSheetName.size() == 0)
        {
            emit sgl_thread_report_check_status(STATUS_ERROR, QString("无法打开文件 %1，路径中禁用特殊符号").arg(xlsxFile.fileName()));
            return;
        }
        // 默认使用第一张表 （表序号从 0 开始）
        xlsx.selectSheet(0);
        emit sgl_thread_report_check_status(STATUS_INFO, QString("开始检查【%1】数据表 【%2】").arg(xlsxFile.fileName(), listSheetName.at(0)), false);

        QXlsx::CellRange range = xlsx.dimension();
        uint16_t rowCount = range.rowCount();
        uint16_t columnCount = range.columnCount();

        // 行数小于 3 行，提示数据模板可能不正确或数据集为空，继续解析剩下的文件
        if (rowCount < 3)
        {
            emit sgl_thread_report_check_status(STATUS_ERROR, QString("表格 【%1】 数据集为空").arg(listSheetName.at(0), false));
            continue;
        }

        // 列数小于 25 列，提示数据模板可能不正确
        if (columnCount < 25)
        {
            emit sgl_thread_report_check_status(STATUS_ERROR, QString("数据表 【%1】 模板可能不正确，列数 %2，小于 25 列").arg(listSheetName.at(0), QString::number(columnCount)));
            return;
        }
    }

    QDir sideScanDir = rootDir.absolutePath() + "/image";
    if (!sideScanDir.exists())
    {
        emit sgl_thread_report_check_status(STATUS_ERROR, QString("侧扫图像文件夹 %1 不存在").arg(sideScanDir.absolutePath()));
        return;
    }

    // 擦汗寻文件夹下存在的图片文件信息
    QFileInfoList listSideScanFile = traverseFolder(sideScanDir.absolutePath());

    // 根据名称获取文件信息
    auto findImage = [&](const QString &fileName)
    {
        for (auto &info : listSideScanFile)
        {
            if (info.fileName() == fileName) return info;
        }

        return QFileInfo();
    };

    QDir navigationDir = rootDir.absolutePath() + "/Navigation";
    if (!navigationDir.exists())
    {
        emit sgl_thread_report_check_status(STATUS_ERROR, QString("轨迹数据文件夹 %1 不存在").arg(navigationDir.absolutePath()));
        return;
    }

    QDir navigationAUVDir = navigationDir.absolutePath() + "/AUV";
    if (!navigationAUVDir.exists())
    {
        emit sgl_thread_report_check_status(STATUS_ERROR, QString("轨迹数据文件夹 %1 不存在").arg(navigationAUVDir.absolutePath()));
        return;
    }
    // 此处是否需要进行非空检索

    QDir navigationDTVDir = navigationDir.absolutePath() + "/DT";
    if (!navigationDTVDir.exists())
    {
        emit sgl_thread_report_check_status(STATUS_ERROR, QString("轨迹数据文件夹 %1 不存在").arg(navigationDTVDir.absolutePath()));
        return;
    }
    // 此处是否需要进行非空检索

    QDir navigationHOVDir = navigationDir.absolutePath() + "/HOV";
    if (!navigationHOVDir.exists())
    {
        emit sgl_thread_report_check_status(STATUS_ERROR, QString("轨迹数据文件夹 %1 不存在").arg(navigationHOVDir.absolutePath()));
        return;
    }
    // 此处是否需要进行非空检索

    QDir navigationSHIPDir = navigationDir.absolutePath() + "/SHIP";
    if (!navigationSHIPDir.exists())
    {
        emit sgl_thread_report_check_status(STATUS_ERROR, QString("轨迹数据文件夹 %1 不存在").arg(navigationSHIPDir.absolutePath()));
        return;
    }
    // 此处是否需要进行非空检索

    // 先锁住任务队列
    std::lock_guard<std::mutex> lockQueue(mMutexQueue);

    SideScanSourceList sideScanSource;

    // 主目录一定要是航次名称（此处可以加格式限制）
    QString cruiseNumber = rootDir.dirName();
    for (auto &xlsxFile : listXlsx)
    {
        QXlsx::Document xlsxDocument(xlsxFile.absoluteFilePath());
        xlsxDocument.selectSheet(0);

        QXlsx::CellRange range = xlsxDocument.dimension();
        uint16_t rowCount = range.rowCount();

        // 开始循环检查每一条数据，并且读取到内存中，有错误就提示错误的位置
        // 记录该文件有多少条有效数据
        uint16_t dataCount = 0;
        for (uint32_t i = 3; i <= rowCount; i++)
        {
            // 循环 25 列，判断是否是存粹的空行
            uint8_t emptyColumnNumber = 0;
            for (uint16_t l = 2; l <= 25; l++)
            {
                QXlsx::Cell *temp = xlsxDocument.cellAt(i, l);
                if ((nullptr == temp) || (temp->value().toString().isEmpty()))
                {
                    emptyColumnNumber++;
                }
            }
            // 纯粹的空行不用解析
            if (emptyColumnNumber >= 24) continue;

            SideScanSource *source = sideScanSource.add_list();
            if (nullptr == source)
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, "系统内存不足，无法创建数据缓存");
                return;
            }

            QXlsx::Cell *cell = nullptr;

            // 编号
            cell = xlsxDocument.cellAt(i, CELL_ID);
            source->set_id(getCellValue(cell).toString().remove('\n').trimmed().toStdString());
            if (source->id().size() <= 0)
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("文件 【%1】 的第【%2】行数据 ID 为空").arg(xlsxFile.fileName(), QString::number(i)));
                return;
            }
            if (source->id().size() >= 32)
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的长度【%2】超过最大阈值 【32】").arg(source->id().data(), QString::number(source->id().size())));
                return;
            }

            //拖体记录时间
            cell = xlsxDocument.cellAt(i, CELL_DT_TIME);
            source->set_dt_time(getCellValue(cell).toString().trimmed().toStdString());
            if (source->dt_time().size() >= 24)
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的拖体时间长度【%2】超过最大阈值 【24】").arg(source->id().data(), QString::number(source->dt_time().size())));
                return;
            }

            // 经度(DDMM.MMMM)
            cell = xlsxDocument.cellAt(i, CELL_LONGITUDE);
            double longitude = getCellValue(cell).toDouble() / 100.00;
            longitude = floor(longitude) + (longitude - floor(longitude)) * 100 / 60.0;
            source->set_longitude(longitude);
            if ((source->longitude() == 0.00) || (source->longitude() >= 180.0) || (source->longitude() <= -180.00))
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的经度转换后数值为【%2】，系统判定为异常值").arg(source->id().data(), QString::number(source->longitude())));
                return;
            }

            // 纬度(DDMM.MMMM)
            cell = xlsxDocument.cellAt(i, CELL_LATITUDE);
            double latitude = getCellValue(cell).toDouble() / 100.00;
            latitude = floor(latitude) + (latitude - floor(latitude)) * 100 / 60.0;
            source->set_latitude(latitude);
            if ((source->latitude() == 0.00) || (source->latitude() >= 90.0) || (source->latitude() <= -90.00))
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的纬度转换后数值为【%2】，系统判定为异常值").arg(source->id().data(), QString::number(source->latitude())));
                return;
            }

            // 拖体速度
            cell = xlsxDocument.cellAt(i, CELL_DT_SPEED);
            source->set_dt_speed(getCellValue(cell).toFloat());

            // 水平距离方向
            cell = xlsxDocument.cellAt(i, CELL_HORIZONTAL_RANGE_DIRECTION);
            source->set_horizontal_range_direction(getCellValue(cell).toString().trimmed().toStdString());
            if (source->horizontal_range_direction().size() >= 16)
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的水平距离方向长度【%2】超过最大阈值 【16】").arg(source->id().data(), QString::number(source->horizontal_range_direction().size())));
                return;
            }

            // 水平距离值
            cell = xlsxDocument.cellAt(i, CELL_HORIZONTAL_RANGE_VALUE);
            source->set_horizontal_range_value(getCellValue(cell).toFloat());

            // 离底高度
            cell = xlsxDocument.cellAt(i, CELL_HEIGHT_FROM_BOTTOM);
            source->set_height_from_bottom(getCellValue(cell).toFloat());

            // 航迹向分辨率
            cell = xlsxDocument.cellAt(i, CELL_T_THETA);
            source->set_r_theat(getCellValue(cell).toFloat());

            // 侧扫图片路径 (按照固定逻辑解析)
            QFileInfo sideScanImageInfo = findImage(QString("%1.tif").arg(source->id().data()));
            if (!sideScanImageInfo.exists())
            {
                // 如果侧扫图片不存在，查询的时候给个点，点的位置由上面的经纬度确定
                source->set_side_scan_image_name("");
            }
            else
            {
                source->set_side_scan_image_name(sideScanImageInfo.fileName().toStdString());
                if (source->side_scan_image_name().size() >= 64)
                {
                    emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的侧扫图片名称长度【%2】超过最大阈值 【64】").arg(source->id().data(), QString::number(source->side_scan_image_name().size())));
                    return;
                }

                DataUploadTask task = {"upload", QString("upload/%1/image").arg(cruiseNumber).toStdString(), sideScanImageInfo.absoluteFilePath().toStdString()};
                mTaskQueue.append(task);
            }

            // 长
            cell = xlsxDocument.cellAt(i, CELL_ALONG_TRACK);
            source->set_along_track(getCellValue(cell).toFloat());

            // 宽
            cell = xlsxDocument.cellAt(i, CELL_ACROSS_TRACK);
            source->set_across_track(getCellValue(cell).toFloat());

            // 备注说明
            cell = xlsxDocument.cellAt(i, CELL_REMARKS);
            source->set_remarks(getCellValue(cell).toString().trimmed().toStdString());
            if (source->remarks().size() >= 256)
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的备注说明长度【%2】超过最大阈值 【256】").arg(source->id().data(), QString::number(source->remarks().size())));
                return;
            }

            // 推测尺寸
            cell = xlsxDocument.cellAt(i, CELL_SUPPOSE_SIZE);
            source->set_suppose_size(getCellValue(cell).toFloat());

            // 优先级
            cell = xlsxDocument.cellAt(i, CELL_PRIORITY);
            source->set_priority(getCellValue(cell).toInt() & 0xff);

            // 查证照片描述
            cell = xlsxDocument.cellAt(i, CELL_IMAGE_DESCRIPTION);
            source->set_image_description(getCellValue(cell).toString().trimmed().toStdString());
            if (source->image_description().size() >= 256)
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的照片描述长度【%2】超过最大阈值 【256】").arg(source->id().data(), QString::number(source->image_description().size())));
                return;
            }

            // 目标经度
            cell = xlsxDocument.cellAt(i, CELL_TARGET_LONGITUDE);
            source->set_target_longitude(getCellValue(cell).toString().toStdString());
            if (source->target_longitude().length() >= 64)
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的目标物经度长度超过阈值【64】").arg(source->id().data()));
                return;
            }

            // 目标纬度
            cell = xlsxDocument.cellAt(i, CELL_TARGET_LATITUDE);
            source->set_target_latitude(getCellValue(cell).toString().toStdString());
            if (source->target_latitude().length() >= 64)
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的目标物纬度长度超过阈值【64】").arg(source->id().data()));
                return;
            }

            // 定位误差
            cell = xlsxDocument.cellAt(i, CELL_POSITION_ERROR);
            source->set_position_error(getCellValue(cell).toString().toStdString());
            if (source->position_error().length() >= 64)
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的定位误差长度超过阈值【64】").arg(source->id().data()));
                return;
            }

            // 航次号
            cell = xlsxDocument.cellAt(i, CELL_CRUISE_NUMBER);
            source->set_cruise_number(getCellValue(cell).toString().trimmed().toStdString());

            // 潜次号
            cell = xlsxDocument.cellAt(i, CELL_DIVE_NUMBER);
            source->set_dive_number(getCellValue(cell).toString().trimmed().toStdString());
            if (source->dive_number().size() >= 64)
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的查证潜次长度【%2】超过最大阈值 【64】").arg(source->id().data(), QString::number(source->dive_number().size())));
                return;
            }

            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// 根据潜次号，才能查询查证的图片名称
            // 查证 AUV 图片路径 需要根据 反斜杠 分类检索
            QString auvImagePaths;
            QString imagePaths;
            if (source->dive_number().size() > 0)
            {
                auto listDiveNumber = QString::fromStdString(source->dive_number()).split("/", Qt::SkipEmptyParts);
                for (auto &diveNumber : listDiveNumber)
                {
                    // 开始查询 AUV 查证图片
                    QString name = QString("%1-%2-AS.jpg").arg(source->id().data(), diveNumber);
                    for (uint16_t i = 1; i <= 10; i++)
                    {
                        if (!name.isEmpty()) i--;
                        if (name.isEmpty()) name = QString("%1-%2-AS-%3.jpg").arg(source->id().data(), diveNumber, QString("%1").arg(i, 2, 10, QLatin1Char('0')));
                        //qDebug() << "AUV照片 " << name;
                        QFileInfo auvImageInfo = findImage(name);
                        name.clear();
                        if (!auvImageInfo.exists()) continue;

                        //名称拼接
                        if (!auvImagePaths.isEmpty()) auvImagePaths.append(";");
                        auvImagePaths.append(auvImageInfo.fileName());

                        DataUploadTask task = {"upload", QString("upload/%1/image").arg(cruiseNumber).toStdString(), auvImageInfo.absoluteFilePath().toStdString()};
                        mTaskQueue.append(task);
                    }

                    // 查证图片路径 需要根据 反斜杠 分类检索
                    name = QString("%1-%2.jpg").arg(source->id().data(), diveNumber);
                    for (uint16_t j = 1; j <= 10; j++)
                    {
                        if (!name.isEmpty()) j--;
                        if (name.isEmpty()) name = QString("%1-%2-%3.jpg").arg(source->id().data(), diveNumber, QString("%1").arg(j, 2, 10, QLatin1Char('0')));
                        //qDebug() << "查找照片 " << name;

                        QFileInfo imageInfo = findImage(name);
                        name.clear();
                        if (!imageInfo.exists()) continue;

                        //名称拼接
                        if (!imagePaths.isEmpty()) imagePaths.append(";");
                        imagePaths.append(imageInfo.fileName());

                        DataUploadTask task = {"upload", QString("upload/%1/image").arg(cruiseNumber).toStdString(), imageInfo.absoluteFilePath().toStdString()};
                        mTaskQueue.append(task);
                    }
                }

                if (auvImagePaths.toUtf8().length() > 256)
                {
                    emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的AUV侧扫图片路径长度【%2】超过最大阈值 【256】").arg(source->id().data(), QString::number(auvImagePaths.toUtf8().size())));
                    return;
                }
                source->set_verify_auv_sss_image_paths(auvImagePaths.toStdString());

                if (imagePaths.toUtf8().length() > 256)
                {
                    emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的查证照片路径长度【%2】超过最大阈值 【256】").arg(source->id().data(), QString::number(imagePaths.toUtf8().size())));
                    return;
                }
                source->set_verify_image_paths(imagePaths.toStdString());
            }
            else
            {
                source->set_verify_auv_sss_image_paths("");
                source->set_verify_image_paths("");
            }
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // 查证时间
            cell = xlsxDocument.cellAt(i, CELL_VERIFY_TIME);
            source->set_verify_time(getCellValue(cell).toString().trimmed().toStdString());
            if (source->side_scan_image_name().size() >= 128)
            {
                emit sgl_thread_report_check_status(STATUS_ERROR, QString("编号 【%1】 的查证时间长度【%2】超过最大阈值 【128】").arg(source->id().data(), QString::number(source->verify_time().size())));
                return;
            }

            // 数据状态
            source->set_status(0);

            // 有效数据数量递增
            dataCount++;
        }

        emit sgl_thread_report_check_status(STATUS_INFO, QString("文件【%1】解析完成，共【%2】 条记录").arg(xlsxFile.fileName(), QString::number(dataCount)), false);
    }

    if (sideScanSource.list_size() == 0) emit sgl_thread_report_check_status(STATUS_INFO, "表格解析结束，未能找到有效异常点数据", false);

    CruiseRouteSourceList routeSourceList;
    // AUV 轨迹文件
    auto listTrack = navigationAUVDir.entryInfoList(QStringList("*.txt"), QDir::Files | QDir::NoSymLinks);
    for (auto &track : listTrack)
    {
        bool status = std::regex_match(track.fileName().toStdString(), std::regex(QString("^%1-([^-]*)(-[0-9][0-9])?.txt$").arg(cruiseNumber).toStdString()));
        if (!status) continue;

        if (track.fileName().toUtf8().length() >= 64)
        {
            emit sgl_thread_report_check_status(STATUS_ERROR, QString("AUV 轨迹文件【%1】名称长度【%2】超过最大阈值【64】").arg(track.fileName(), QString::number(track.fileName().toUtf8().length())));
            return;
        }

        DataUploadTask task = {"upload", QString("upload/%1/Navigation/AUV").arg(cruiseNumber).toStdString(), track.absoluteFilePath().toStdString()};
        mTaskQueue.append(task);

        CruiseRouteSource *source = routeSourceList.add_list();
        source->set_cruise(cruiseNumber.toStdString());
        source->set_type("AUV");
        source->set_name(track.fileName().toStdString());
    }

    // DTV 轨迹文件
    listTrack = navigationDTVDir.entryInfoList(QStringList("*.txt"), QDir::Files | QDir::NoSymLinks);
    for (auto &track : listTrack)
    {
        bool status = std::regex_match(track.fileName().toStdString(), std::regex(QString("^%1-([^-]*)(-[0-9][0-9])?.txt$").arg(cruiseNumber).toStdString()));
        if (!status) continue;

        DataUploadTask task = {"upload", QString("upload/%1/Navigation/DT").arg(cruiseNumber).toStdString(), track.absoluteFilePath().toStdString()};
        mTaskQueue.append(task);

        if (track.fileName().toUtf8().length() >= 64)
        {
            emit sgl_thread_report_check_status(STATUS_ERROR, QString("DT 轨迹文件【%1】名称长度【%2】超过最大阈值【64】").arg(track.fileName(), QString::number(track.fileName().toUtf8().length())));
            return;
        }

        CruiseRouteSource *source = routeSourceList.add_list();
        source->set_cruise(cruiseNumber.toStdString());
        source->set_type("DT");
        source->set_name(track.fileName().toStdString());
    }

    // HOV 轨迹文件
    listTrack = navigationHOVDir.entryInfoList(QStringList("*.txt"), QDir::Files | QDir::NoSymLinks);
    for (auto &track : listTrack)
    {
        bool status = std::regex_match(track.fileName().toStdString(), std::regex(QString("^%1-([^-]*)(-[0-9][0-9])?.txt$").arg(cruiseNumber).toStdString()));
        if (!status) continue;

        DataUploadTask task = {"upload", QString("upload/%1/Navigation/HOV").arg(cruiseNumber).toStdString(), track.absoluteFilePath().toStdString()};
        mTaskQueue.append(task);

        if (track.fileName().toUtf8().length() >= 64)
        {
            emit sgl_thread_report_check_status(STATUS_ERROR, QString("HOV 轨迹文件【%1】名称长度【%2】超过最大阈值【64】").arg(track.fileName(), QString::number(track.fileName().toUtf8().length())));
            return;
        }

        CruiseRouteSource *source = routeSourceList.add_list();
        source->set_cruise(cruiseNumber.toStdString());
        source->set_type("HOV");
        source->set_name(track.fileName().toStdString());
    }

    // SHIP 轨迹文件
    listTrack = navigationSHIPDir.entryInfoList(QStringList("*.txt"), QDir::Files | QDir::NoSymLinks);
    for (auto &track : listTrack)
    {
        bool status = std::regex_match(track.fileName().toStdString(), std::regex(QString("^%1-([^-]*)(-[0-9][0-9])?.txt$").arg(cruiseNumber).toStdString()));
        if (!status) continue;

        DataUploadTask task = {"upload", QString("upload/%1/Navigation/SHIP").arg(cruiseNumber).toStdString(), track.absoluteFilePath().toStdString()};
        mTaskQueue.append(task);

        if (track.fileName().toUtf8().length() >= 64)
        {
            emit sgl_thread_report_check_status(STATUS_ERROR, QString("SHIP 轨迹文件【%1】名称长度【%2】超过最大阈值【64】").arg(track.fileName(), QString::number(track.fileName().toUtf8().length())));
            return;
        }

        CruiseRouteSource *source = routeSourceList.add_list();
        source->set_cruise(cruiseNumber.toStdString());
        source->set_type("SHIP");
        source->set_name(track.fileName().toStdString());
    }

    uint8_t increaceTaskNumber = 0;
    if (sideScanSource.list_size() > 0)
    {
        DataUploadTask taskSideScan = {"insert", sideScanSource.SerializeAsString(), "", CMD_INSERT_SIDE_SCAN_SOURCE_DATA};
        mTaskQueue.append(taskSideScan);
        increaceTaskNumber++;
    }

    if (routeSourceList.list_size() > 0)
    {
        DataUploadTask taskRouteSource = {"insert", routeSourceList.SerializeAsString(), "", CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA};
        mTaskQueue.append(taskRouteSource);
        increaceTaskNumber++;
    }

    emit sgl_thread_report_check_status(STATUS_INFO, QString("文件夹解析结束，共【%1】条异常点记录，【%2】个轨迹文件").arg(QString::number(sideScanSource.list_size()), QString::number(mTaskQueue.size() - increaceTaskNumber)), false);

    emit sgl_thread_check_data_finish();
}

QFileInfoList DialogUploadData::traverseFolder(const QString &dir)
{
    QDir root(dir);
    if (!root.exists()) return QFileInfoList();

    QFileInfoList listFile = root.entryInfoList({"*.tif", "*.TIF", "*.jpg"}, QDir::Files | QDir::NoDotAndDotDot);

    QFileInfoList listDirs = root.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);

    for (auto &subDir : listDirs)
    {
        listFile.append(traverseFolder(subDir.absoluteFilePath()));
    }

    // qDebug() << "listFileB " << dir << " " << listFile;

    return listFile;
}

void DialogUploadData::slot_btn_upload_data_click()
{
    if (mRunThreadCheck)
    {
        emit sgl_send_system_notice_message(STATUS_INFO, "请等待当前数据解析完成");
        return;
    }

    if (!mTcpServerFlag)
    {
        emit sgl_send_system_notice_message(STATUS_INFO, "请检查数据服务状态");
        if (nullptr == mTcpSocket) return;

        emit sgl_send_system_notice_message(STATUS_INFO, "尝试连接远程数据服务 ...");
        mTcpSocket->connect();
        return;
    }

    if (!mFtpServerFlag)
    {
        emit sgl_send_system_notice_message(STATUS_INFO, "请检查文件服务状态");
        return;
    }

    if (mTaskQueue.size() > 0)
    {
        emit sgl_send_system_notice_message(STATUS_INFO, "请等待当前数据录入完成");
        return;
    }

    std::lock_guard<std::mutex> lock(mMutexCheck);
    mRunThreadCheck = true;

    auto func = std::bind(&DialogUploadData::checkData, this);
    std::thread th(func);
    th.detach();
}

void DialogUploadData::slot_btn_select_dir_click()
{
    QString desk = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择文件夹"), desk, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) return;

    ui->tbRootDir->setText(dir);
}

void DialogUploadData::slot_thread_report_check_status(uint8_t status, const QString &msg, bool stop)
{
    QString value;
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    if (status == STATUS_SUCCESS)
    {
        value = QString("<p style=\"font-family: Microsoft YaHei UI; color:#666666\"> %1 成功: <span style=\"color:#3f8f54\"> &nbsp; %2</span></p>").arg(time, msg);
    }
    else if (status == STATUS_INFO)
    {
        value = QString("<p style=\"font-family: Microsoft YaHei UI; color:#666666\"> %1 提示: <span style=\"color:#f86c2e\"> &nbsp; %2</span></p>").arg(time, msg);
    }
    else
    {
        value = QString("<p style=\"font-family: Microsoft YaHei UI; color:#666666\"> %1 错误: <span style=\"color:#dd3737\"> &nbsp; %2</span></p>").arg(time, msg);
    }
    ui->textEditUploadStatus->append(value);

    if (stop)
    {
        std::lock_guard<std::mutex> lock(mMutexCheck);
        mRunThreadCheck = false;

        std::lock_guard<std::mutex> lockQueue(mMutexQueue);
        mTaskQueue.clear();
        emit sgl_thread_report_check_status(STATUS_INFO, "数据录入流程中止", false);
        emit sgl_thread_report_check_status(STATUS_INFO, QString(72, '-'), false);
    }
}

void DialogUploadData::slot_thread_check_data_finish()
{
    mRunThreadCheck = false;

    if (nullptr == mFtpManager)
    {
        mFtpManager = new FtpManager;
        mFtpManager->setFtpHost("101.34.253.220");
        mFtpManager->setFtpUserName("idsse");
        mFtpManager->setFtpUserPass("123456");

        connect(mFtpManager, &FtpManager::sgl_file_upload_process, this, [this](const QString &file, float percent)
        {
            if (percent == 100)
            {
                emit sgl_thread_report_check_status(STATUS_SUCCESS, QString("上传文件成功 %1").arg(file), false);

                emit sgl_start_next_task();
            }
        });
    }

//        connect(ftpManager, &FtpManager::sgl_ftp_task_response, this, &MainWindow::slot_ftp_task_response);

    // 直接使用 FTP 类开始上传图片文件，实时发送上传结果到界面（不需要进度，文件较小），提前新建一个航次名称对应的文件夹（便于后期管理员查看）
    // 如果失败，发送删除？？？
    // 全部成功则给出提示
    emit sgl_thread_report_check_status(STATUS_INFO, QString("开始数据上传任务"), false);

    emit sgl_start_next_task();

    // 直接使用 FTP 类开始上传轨迹文件，实时发送上传结果到界面（不需要进度，文件较小）
    // 如果失败，发送删除？？？
    // 全部成功则给出提示

    // 发送异常点数据到服务器，服务器程序执行数据录入工作（开启事务）
    // 如果录入成功，返回成功的结果，否则，给出 MySQL 返回的错误结果信息并开始回滚 （不提交事务）

    // 发送航次轨迹文件路径数据到服务器，服务器程序执行数据录入工作（开启事务）
    // 如果录入成功，返回成功的结果，否则，给出 MySQL 返回的错误结果信息并开始回滚 （提交事务）

    // 发送数据录入完成的结果，可以包括数量等信息

    // 测试直接跳出

    // 最后修改录入状态
}

void DialogUploadData::slot_recv_socket_data(uint64_t dwconnid, const std::string &data)
{
    Q_UNUSED(dwconnid);
    uint32_t cmd = 0;
    memcpy(&cmd, data.data() + 2, 2);

    switch (cmd) {
    case CMD_INSERT_SIDE_SCAN_SOURCE_DATA_RESPONSE:
    {
        StatusResponse response;
        response.ParseFromString(data.substr(8));

        if (response.status())
        {
            slot_thread_report_check_status(STATUS_SUCCESS, QString("异常点数据录入成功 %1").arg(response.message().data()), false);
            // 此处可以尝试开启下一个任务
            emit sgl_start_next_task();
        }
        else
        {
            slot_thread_report_check_status(STATUS_ERROR, QString("异常点数据录入失败 %1").arg(response.message().data()), false);

            std::lock_guard<std::mutex> lock(mMutexQueue);
            mTaskQueue.clear();
            emit sgl_thread_report_check_status(STATUS_INFO, "数据录入流程中止", false);
            emit sgl_thread_report_check_status(STATUS_INFO, QString(72, '-'), false);
        }
        break;
    }
    case CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA_RESPONSE:
    {
        StatusResponse response;
        response.ParseFromString(data.substr(8));

        if (response.status())
        {
            slot_thread_report_check_status(STATUS_SUCCESS, QString("轨迹数据录入成功 %1").arg(response.message().data()), false);
            // 此处可以尝试开启下一个任务
            emit sgl_start_next_task();
        }
        else
        {
            slot_thread_report_check_status(STATUS_ERROR, QString("轨迹点数据录入失败 %1").arg(response.message().data()), false);

            std::lock_guard<std::mutex> lock(mMutexQueue);
            mTaskQueue.clear();
            emit sgl_thread_report_check_status(STATUS_INFO, "数据录入流程中止", false);
            emit sgl_thread_report_check_status(STATUS_INFO, QString(72, '-'), false);
        }
        break;
    }
    case CMD_QUERY_FTP_SERVER_STATUS_RESPONSE:
    {
        StatusResponse response;
        response.ParseFromString(data.substr(8));
        slot_thread_report_check_status(response.status() ? STATUS_SUCCESS : STATUS_ERROR, response.message().data(), false);

        // 文件服务状态
        mFtpServerFlag = response.status();
        break;
    }
    default:
        break;
    }
}

void DialogUploadData::slot_start_next_task()
{
    if (mTaskQueue.isEmpty())
    {
        emit sgl_thread_report_check_status(STATUS_INFO, QString("数据录入流程结束"), false);
        emit sgl_thread_report_check_status(STATUS_INFO, QString(72, '-'), false);
        return;
    }

    std::lock_guard<std::mutex> lock(mMutexQueue);
    DataUploadTask task = mTaskQueue.takeFirst();

    // 进行文件上传任务
    if (task.type == "upload")
    {
        if (nullptr == mFtpManager)
        {
            // 清理
            mTaskQueue.clear();
            emit sgl_thread_report_check_status(STATUS_ERROR, QString("系统错误，请联系管理员"), false);
            return;
        }

        if (!mFtpServerFlag)
        {
            // 清理
            mTaskQueue.clear();
            emit sgl_thread_report_check_status(STATUS_ERROR, QString("文件服务关闭，请联系管理员"), false);
            return;
        }
        // 进行文件上传任务
        mFtpManager->uploadFile(task.arg2.data(), task.arg1.data());
    }
    // 进行数据写入任务
    else if (task.type == "insert")
    {
        if (nullptr == mTcpSocket)
        {
            // 清理
            mTaskQueue.clear();
            emit sgl_thread_report_check_status(STATUS_ERROR, QString("系统错误，请联系管理员"), false);
            return;
        }

        if (!mTcpServerFlag)
        {
            // 清理
            mTaskQueue.clear();
            emit sgl_thread_report_check_status(STATUS_ERROR, QString("数据服务关闭，请联系管理员"), false);
            return;
        }

        QByteArray message = createPackage(task.arg3, QByteArray::fromStdString(task.arg1));
        mTcpSocket->write(message.toStdString());
    }
}
