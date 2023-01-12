#include "widgetselectparameter.h"
#include "ui_widgetselectparameter.h"
#include "Common/common.h"
#include "Protocol/protocolhelper.h"
#include "Public/appconfig.h"
#include "Net/usernetworker.h"
#include "Public/appsignal.h"

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

void WidgetSelectParameter::requestSelectParameter()
{
    QByteArray pack = ProtocolHelper::getInstance()->createPackage(CMD_QUERY_SEARCH_FILTER_PARAMETER_DATA);
    UserNetWorker::getInstance()->sendPack(pack);
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

    connect(AppSignal::getInstance(), &AppSignal::sgl_query_search_filter_parameter_response, this, &WidgetSelectParameter::slot_query_search_filter_parameter_response);
}

void WidgetSelectParameter::slot_query_search_filter_parameter_response(const SearchFilterParamterList &response)
{
    int size = response.list_size();

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
    }

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
}
