#include "widgetselectparameter.h"
#include "ui_widgetselectparameter.h"
#include "Net/tcpsocket.h"
#include "Common/common.h"
#include "Protocol/protocolhelper.h"
#include "Proto/sidescansource.pb.h"

#include <QPushButton>

WidgetSelectParameter::WidgetSelectParameter(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetSelectParameter)
{
    ui->setupUi(this);

    init();
}

WidgetSelectParameter::~WidgetSelectParameter()
{
    delete ui;
}

void WidgetSelectParameter::init()
{
    connect(ui->btnPriority_1, &QPushButton::toggled, this, [this](bool checked)
    {
        emit sgl_modify_search_parameter("priority", ((QPushButton*)sender())->text(), checked);
    });
    connect(ui->btnPriority_2, &QPushButton::toggled, this, [this](bool checked)
    {
        emit sgl_modify_search_parameter("priority", ((QPushButton*)sender())->text(), checked);
    });
    connect(ui->btnPriority_3, &QPushButton::toggled, this, [this](bool checked)
    {
        emit sgl_modify_search_parameter("priority", ((QPushButton*)sender())->text(), checked);
    });
    connect(ui->btnVerify_1, &QPushButton::toggled, this, [this](bool checked)
    {
        emit sgl_modify_search_parameter("verify_flag", "1", checked);
    });
    connect(ui->btnVerify_0, &QPushButton::toggled, this, [this](bool checked)
    {
        emit sgl_modify_search_parameter("verify_flag", "0", checked);
    });

    // 准备执行 SQL 任务
    mTcpSocket = new TcpSocket;
    connect(mTcpSocket, &TcpSocket::sgl_recv_socket_data, this, &WidgetSelectParameter::slot_recv_socket_data);
    connect(mTcpSocket, &TcpSocket::sgl_tcp_socket_connect, this, &WidgetSelectParameter::slot_tcp_socket_connect);
    connect(mTcpSocket, &TcpSocket::sgl_tcp_socket_disconnect, this, &WidgetSelectParameter::slot_tcp_socket_disconnect);
    mTcpSocket->connect("101.34.253.220", 60011);
}

void WidgetSelectParameter::slot_tcp_socket_connect(uint64_t dwconnid)
{
    Q_UNUSED(dwconnid);
    mTcpSocketConnected = true;
    if (nullptr == mTcpSocket) return;

    QByteArray pack = ProtocolHelper::getInstance()->createPackage(CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA);
    mTcpSocket->write(pack.toStdString());
}

void WidgetSelectParameter::slot_recv_socket_data(uint64_t dwconnid, const std::string &data)
{
    Q_UNUSED(dwconnid);

    uint32_t cmd = 0;
    memcpy(&cmd, data.data() + 2, 2);

    switch (cmd) {
    case CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA_RESPONSE:
    {
        SearchFilterParamterList response;
        bool status = response.ParseFromString(data.substr(8));
        if (!status)
        {
            qDebug() << "数据解析错误，请联系管理员";
            return;
        }
        qDebug() << "slot_recv_socket_data " << response.list_size();

        int size = response.list_size();
        // 动态修改时间的按钮个数
        auto listChildrenCruiseYear = ui->widgetCruiseYear->findChildren<QPushButton*>();
        for (auto &button : listChildrenCruiseYear)
        {

        }

        QStringList listCruiseYear;
        QStringList listCruiseNumber;
        QStringList listDiveNumber;
        QStringList listVerifyDiveNumber;
        for (int i = 0; i < size; i++)
        {
            QString year = response.list().at(i).cruise_year().data();
            if(!listCruiseYear.contains(year)) listCruiseYear.append(year);

            QString cruiseNumber = response.list().at(i).cruise_number().data();
            if(!listCruiseNumber.contains(cruiseNumber)) listCruiseNumber.append(cruiseNumber);

            QString diveNumber = response.list().at(i).dive_number().data();
            if(!listDiveNumber.contains(diveNumber)) listDiveNumber.append(diveNumber);

            QString verifyDiveNumber = response.list().at(i).verify_dive_number().data();
            auto list = verifyDiveNumber.split("/", Qt::SkipEmptyParts);
            for (auto &diveNumber : list)
            {
                if(!listVerifyDiveNumber.contains(diveNumber)) listVerifyDiveNumber.append(diveNumber);
            }
            qDebug() << "verifyDiveNumber " << verifyDiveNumber;
        }

        qDebug() << "listCruiseYear " << listCruiseYear.size();
        qDebug() << "listCruiseNumber " << listCruiseNumber.size();
        qDebug() << "listDiveNumber " << listDiveNumber.size();
        qDebug() << "listVerifyDiveNumber " << listVerifyDiveNumber.size();

        for (int i = 0; i < listCruiseYear.size(); i++)
        {
            QPushButton *button = new QPushButton(listCruiseYear.at(i));
            button->setCheckable(true);
            connect(button, &QPushButton::toggled, this, [this](bool checked)
            {
                emit sgl_modify_search_parameter("cruise_year", ((QPushButton*)sender())->text(), checked);
            });
            button->setProperty("SelectSearchParameter", true);
            ((QGridLayout*)ui->widgetCruiseYear->layout())->addWidget(button, i / 3, i % 3);
        }

        for (int i = 0; i < listCruiseNumber.size(); i++)
        {
            QPushButton *button = new QPushButton(listCruiseNumber.at(i));
            button->setCheckable(true);
            connect(button, &QPushButton::toggled, this, [this](bool checked)
            {
                emit sgl_modify_search_parameter("cruise_number", ((QPushButton*)sender())->text(), checked);
            });
            button->setProperty("SelectSearchParameter", true);
            ((QGridLayout*)ui->widgetCruiseNumber->layout())->addWidget(button, i / 3, i % 3);
        }

        for (int i = 0; i < listDiveNumber.size(); i++)
        {
            QPushButton *button = new QPushButton(listDiveNumber.at(i));
            button->setCheckable(true);
            connect(button, &QPushButton::toggled, this, [this](bool checked)
            {
                emit sgl_modify_search_parameter("dive_number", ((QPushButton*)sender())->text(), checked);
            });
            button->setProperty("SelectSearchParameter", true);
            ((QGridLayout*)ui->widgetDiveNumber->layout())->addWidget(button, i / 3, i % 3);
        }

        for (int i = 0; i < listVerifyDiveNumber.size(); i++)
        {
            QPushButton *button = new QPushButton(listVerifyDiveNumber.at(i));
            button->setCheckable(true);
            button->setProperty("SelectSearchParameter", true);
            connect(button, &QPushButton::toggled, this, [this](bool checked)
            {
                emit sgl_modify_search_parameter("verify_dive_number", ((QPushButton*)sender())->text(), checked);
            });
            ((QGridLayout*)ui->widgetVerifyDiveNumber->layout())->addWidget(button, i / 3, i % 3);
        }


        break;
    }
    default:
        break;
    }
}

void WidgetSelectParameter::slot_tcp_socket_disconnect(uint64_t dwconnid)
{
    Q_UNUSED(dwconnid);
    mTcpSocketConnected = false;
}
