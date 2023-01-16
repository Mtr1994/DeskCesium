#include "protocolhelper.h"
#include "Common/common.h"
#include "Public/appsignal.h"

#include "Proto/sidescansource.pb.h"

#include <QFile>
#include <QDateTime>
#include <thread>
#include <QDir>
#include <fstream>
#include <QTextCodec>
#include <regex>

using namespace std;

// test
#include <QDebug>

ProtocolHelper::ProtocolHelper(QObject *parent)
    : QObject{parent}
{

}

ProtocolHelper *ProtocolHelper::getInstance()
{
    static ProtocolHelper protocolHelper;
    return &protocolHelper;
}

void ProtocolHelper::init()
{
    // 开启一个数据包解析线程
    auto func = std::bind(&ProtocolHelper::parse, this);
    std::thread th(func);
    th.detach();
}

QByteArray ProtocolHelper::createPackage(uint32_t cmd, const std::string &para)
{
    uint64_t size = para.length();
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
    memcpy(array.data() + 8, para.data(), size);
    return array;
}

void ProtocolHelper::parsePackage(const QByteArray &data)
{
    //qDebug() << "parsePackage " << data.toHex();
    std::unique_lock<std::mutex> lock(mBufferMutex);
    mBufferArray.append(data);
    mCvPackParse.notify_one();
}

void ProtocolHelper::parse()
{
    mParsePackage = true;

    std::mutex mutexParse;
    while (mParsePackage)
    {
        std::unique_lock<std::mutex> lockParse(mutexParse);
        mCvPackParse.wait(lockParse, [this]
        {
            int64_t len = mBufferArray.size();
            return (len >= 8) && (len > mCurrentPackSize);
        });

        std::unique_lock<std::mutex> lockBuffer(mBufferMutex);

        uint32_t cmd = 0;
        memcpy(&cmd, mBufferArray.data() + 2, 2);

        int64_t size = 0;
        memcpy(&size, mBufferArray.data() + 4, 4);

        mCurrentPackSize = size;

        qDebug() << "ProtocolHelper::parse " << cmd << " " << size;

        if (size > (mBufferArray.size() - 8)) continue;

        // 不能超过最大阈值 1000 kb
        if (mBufferArray.size() > 1024000)
        {
            mBufferArray.clear();
            continue;
        }

        uint64_t packLength = size + 8;
        std::string pack = mBufferArray.mid(0, packLength).toStdString();
        mBufferArray = mBufferArray.right(mBufferArray.size() - packLength);
        lockBuffer.unlock();

        qDebug() << "Package ID " << cmd << " " << size;

        switch (cmd) {
        case CMD_QUERY_FTP_SERVER_STATUS_RESPONSE:
        {
            StatusResponse response;
            bool status = response.ParseFromString(pack.substr(8));
            if (!status)
            {
                qDebug() << "数据包 CMD_QUERY_FTP_SERVER_STATUS_RESPONSE 解析异常";
                continue;
            }
            emit AppSignal::getInstance()->sgl_ftp_server_work_status(response.status(), response.message().data());
            break;
        }
        case CMD_INSERT_SIDE_SCAN_SOURCE_DATA_RESPONSE:
        {
            StatusResponse response;
            bool status = response.ParseFromString(pack.substr(8));
            if (!status)
            {
                qDebug() << "数据包 CMD_INSERT_SIDE_SCAN_SOURCE_DATA_RESPONSE 解析异常";
                continue;
            }
            emit AppSignal::getInstance()->sgl_insert_side_scan_source_data_response(response.status(), response.message().data());
            break;
        }
        case CMD_INSERT_CRUISE_ROUTE_SOURCE_DATA_RESPONSE:
        {
            StatusResponse response;
            bool status = response.ParseFromString(pack.substr(8));
            if (!status)
            {
                qDebug() << "数据包 CMD_INSERT_SIDE_SCAN_SOURCE_DATA_RESPONSE 解析异常";
                continue;
            }
            emit AppSignal::getInstance()->sgl_insert_cruise_route_source_data_response(response.status(), response.message().data());
            break;
        }
        case CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_FILTER_RESPONSE:
        case CMD_QUERY_SIDE_SCAN_SOURCE_DATA_BY_KEYWORD_RESPONSE:
        {
            SideScanSourceList response;
            bool status = response.ParseFromString(pack.substr(8));
            if (!status)
            {
                qDebug() << "数据包 CMD_QUERY_SIDE_SCAN_SOURCE_DATA_RESPONSE 解析异常";
                return;
            }

            int size = response.list_size();

            QList<QStringList> list;
            for(int i = 0; i < size; i++)
            {
                QStringList listItem;
                listItem.append(response.list().at(i).id().data());
                listItem.append(response.list().at(i).cruise_number().data());
                listItem.append(response.list().at(i).dive_number().data());
                listItem.append(response.list().at(i).scan_line().data());
                listItem.append(response.list().at(i).cruise_year().data());
                listItem.append(response.list().at(i).dt_time().data());
                listItem.append(QString::number(response.list().at(i).longitude(), 'f', 6));
                listItem.append(QString::number(response.list().at(i).latitude(), 'f', 6));
                listItem.append(QString::number(response.list().at(i).depth(), 'f', 2));
                listItem.append(QString::number(response.list().at(i).dt_speed(), 'f', 2));
                listItem.append(response.list().at(i).horizontal_range_direction().data());
                listItem.append(response.list().at(i).horizontal_range_value().data());
                listItem.append(QString::number(response.list().at(i).height_from_bottom(), 'f', 2));
                listItem.append(QString::number(response.list().at(i).r_theta(), 'f', 2));
                listItem.append(response.list().at(i).side_scan_image_name().data());
                listItem.append(QString::number(response.list().at(i).image_top_left_longitude(), 'f', 6));
                listItem.append(QString::number(response.list().at(i).image_top_left_latitude(), 'f', 6));
                listItem.append(QString::number(response.list().at(i).image_bottom_right_longitude(), 'f', 6));
                listItem.append(QString::number(response.list().at(i).image_bottom_right_latitude(), 'f', 6));
                listItem.append(QString::number(response.list().at(i).image_total_byte()));
                listItem.append(QString::number(response.list().at(i).along_track(), 'f', 2));
                listItem.append(QString::number(response.list().at(i).across_track(), 'f', 2));
                listItem.append(response.list().at(i).remarks().data());
                listItem.append(response.list().at(i).suppose_size().data());
                listItem.append(QString::number(response.list().at(i).priority()));
                listItem.append(response.list().at(i).verify_auv_sss_image_paths().data());
                listItem.append(response.list().at(i).verify_image_paths().data());
                listItem.append(response.list().at(i).image_description().data());
                listItem.append(response.list().at(i).target_longitude().data());
                listItem.append(response.list().at(i).target_latitude().data());
                listItem.append(response.list().at(i).position_error().data());
                listItem.append(response.list().at(i).verify_cruise_number().data());
                listItem.append(response.list().at(i).verify_dive_number().data());
                listItem.append(response.list().at(i).verify_time().data());
                listItem.append(response.list().at(i).verify_flag() ? "已查证" : "未查证");
                listItem.append(QString::number(response.list().at(i).status_flag()));

                list.append(listItem);
            }
            emit AppSignal::getInstance()->sgl_query_side_scan_source_data_response(list);
            break;
        }
        case CMD_QUERY_TRAJECTORY_BY_CURSE_AND_DIVE_RESPONSE:
        {
            RequestTrajectoryResponse response;
            bool status = response.ParseFromString(pack.substr(8));

            if (!status)
            {
                qDebug() << "数据包 CMD_QUERY_TRAJECTORY_BY_CURSE_AND_DIVE_RESPONSE 解析异常";
                continue;
            }
            emit AppSignal::getInstance()->sgl_query_trajectory_data_response(response);
            break;
        }
        case CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA_RESPONSE:
        {
            SearchFilterParamterList response;
            bool status = response.ParseFromString(pack.substr(8));

            if (!status)
            {
                qDebug() << "数据包 CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA_RESPONSE 解析异常";
                continue;
            }
            emit AppSignal::getInstance()->sgl_query_search_filter_parameter_response(response);
            break;
        }
        case CMD_QUERY_STATISTICS_DATA_BY_CONDITION_RESPONSE:
        {
            RequestStatisticsResponse response;
            bool status = response.ParseFromString(pack.substr(8));

            if (!status)
            {
                qDebug() << "数据包 CMD_QUERY_STATISTICS_DATA_BY_CONDITION_RESPONSE 解析异常";
                continue;
            }
            emit AppSignal::getInstance()->sgl_query_statistics_data_by_condition_response(response);
            break;
        }

        default:
            qDebug() << "未处理的数据包 " << cmd;
            break;
        }

        mCurrentPackSize = 0;
    }

    qDebug() << "parse pack finish ";
}
