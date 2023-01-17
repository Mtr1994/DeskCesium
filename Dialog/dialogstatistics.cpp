#include "dialogstatistics.h"
#include "ui_dialogstatistics.h"
#include "Channel/surveycontext.h"
#include "Channel/chartcontext.h"
#include "Channel/prefacejscontext.h"
#include "Protocol/protocolhelper.h"
#include "Net/usernetworker.h"
#include "Common/common.h"
#include "Public/appsignal.h"

#include <QWebEngineSettings>
#include <QWebChannel>

// test
#include <QDebug>

DialogStatistics::DialogStatistics(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogStatistics)
{
    ui->setupUi(this);

    init();
}

DialogStatistics::~DialogStatistics()
{
    delete ui;
}

void DialogStatistics::init()
{
    setWindowTitle("专项调查任务统计");

    //setMinimumHeight(nativeParentWidget()->height());
    resize(width(), nativeParentWidget()->height());
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

    connect(AppSignal::getInstance(), &AppSignal::sgl_query_statistics_data_by_condition_response, this, &DialogStatistics::slot_query_statistics_data_by_condition_response);

    QString htmlRoot = QApplication::applicationDirPath() + "/../Resource/html";
    //QString htmlRoot = QApplication::applicationDirPath() + "/resource/html";

    mJsContextPreface = new PrefaceJsContext(this);
    QWebChannel *channelPreface = new QWebChannel(this);
    channelPreface->registerObject("context", mJsContextPreface);
    ui->widgetWebPreface->page()->setWebChannel(channelPreface);

    ui->widgetWebPreface->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    ui->widgetWebPreface->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    ui->widgetWebPreface->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    ui->widgetWebPreface->page()->setWebChannel(channelPreface);
    ui->widgetWebPreface->page()->load(QUrl(QString("%1/preface.html").arg(htmlRoot)));
    ui->widgetWebPreface->page()->setBackgroundColor(QColor(255, 255, 255));

    connect(mJsContextPreface, &PrefaceJsContext::sgl_web_view_init_finish, this, [this]{ requestStatisticsData(); });

    mSurveyContext = new SurveyContext(this);
    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject("context", mSurveyContext);
    ui->widgetStatisticsCesium->page()->setWebChannel(channel);

    // Cesium 组件
    ui->widgetStatisticsCesium->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    ui->widgetStatisticsCesium->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    ui->widgetStatisticsCesium->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    ui->widgetStatisticsCesium->page()->setWebChannel(channel);
    ui->widgetStatisticsCesium->page()->load(QUrl(QString("%1/indexStatisticsCesium.html").arg(htmlRoot)));
    ui->widgetStatisticsCesium->page()->setBackgroundColor(QColor(0, 0, 0));

    connect(mSurveyContext, &SurveyContext::sgl_web_view_init_finish, this, [this]{ requestStatisticsData(); });

    mJsContextChartYear = new ChartContext(this);
    QWebChannel *channelChartYear = new QWebChannel(this);
    channelChartYear->registerObject("context", mJsContextChartYear);

    // 折线图 年份
    ui->widgetChartCurveYear->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    ui->widgetChartCurveYear->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    ui->widgetChartCurveYear->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    ui->widgetChartCurveYear->page()->setWebChannel(channelChartYear);
    ui->widgetChartCurveYear->page()->load(QUrl(QString("%1/indexStatisticsChart.html").arg(htmlRoot)));
    ui->widgetChartCurveYear->page()->setBackgroundColor(QColor(255, 255, 255));

    connect(mJsContextChartYear, &ChartContext::sgl_web_view_init_finish, this, [this]{ requestStatisticsData(); });
    //connect(mJsContextChartYear, &ChartContext::sgl_web_view_init_finish, this, [this]{ emit mJsContextChartYear->sgl_load_year_curve_chart("{year: ['2021', '2022'], value: [46, 78]}"); });

    // 饼图 查证
    mJsContextChartChecked = new ChartContext(this);
    QWebChannel *channelChartChecked = new QWebChannel(this);
    channelChartChecked->registerObject("context", mJsContextChartChecked);
    ui->widgetChartPieChecked->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    ui->widgetChartPieChecked->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    ui->widgetChartPieChecked->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    ui->widgetChartPieChecked->page()->setWebChannel(channelChartChecked);
    ui->widgetChartPieChecked->page()->load(QUrl(QString("%1/indexStatisticsChart.html").arg(htmlRoot)));
    ui->widgetChartPieChecked->page()->setBackgroundColor(QColor(255, 255, 255));

    connect(mJsContextChartChecked, &ChartContext::sgl_web_view_init_finish, this, [this]{ requestStatisticsData(); });
    //connect(mJsContextChartChecked, &ChartContext::sgl_web_view_init_finish, this, [this]{ emit mJsContextChartChecked->sgl_load_check_pie_chart("[{value: 70, name: '已查证'}, {value: 30, name: '未查证'}]"); });

    // 饼图 优先级
    mJsContextChartPriority = new ChartContext(this);
    QWebChannel *channelChartPriority = new QWebChannel(this);
    channelChartPriority->registerObject("context", mJsContextChartPriority);
    ui->widgetChartPiePriority->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    ui->widgetChartPiePriority->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    ui->widgetChartPiePriority->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    ui->widgetChartPiePriority->page()->setWebChannel(channelChartPriority);
    ui->widgetChartPiePriority->page()->load(QUrl(QString("%1/indexStatisticsChart.html").arg(htmlRoot)));
    ui->widgetChartPiePriority->page()->setBackgroundColor(QColor(255, 255, 255));

//    ////// 网页调试部分，发布时请注释此段代码 S
//    QWebEngineView *debugPage = new QWebEngineView;
//    ui->widgetWebPreface->page()->setDevToolsPage(debugPage->page());
//    ui->widgetWebPreface->page()->triggerAction(QWebEnginePage::WebAction::InspectElement);
//    debugPage->show();
//    ////// 网页调试部分，发布时请注释此段代码 E

    connect(mJsContextChartPriority, &ChartContext::sgl_web_view_init_finish, this, [this]{ requestStatisticsData(); });
    //connect(mJsContextChartPriority, &ChartContext::sgl_web_view_init_finish, this, [this]{ emit mJsContextChartPriority->sgl_load_priority_pie_chart("[{value: 24, name: '优先级1'}, {value: 46, name: '优先级2'}, {value: 30, name: '优先级3'}]"); });
}

void DialogStatistics::requestStatisticsData()
{
    mNumberInitWebView++;
    if (mNumberInitWebView < 5) return;
    qDebug() << "DialogStatistics::requestStatisticsData Start";

    RequestStatistics requestStatistics;
    requestStatistics.set_query_dt(true);
    requestStatistics.set_query_auv(true);
    requestStatistics.set_query_errorpoint(true);
    requestStatistics.set_query_hov(true);
    requestStatistics.set_query_preface(true);
    requestStatistics.set_query_ship(true);
    requestStatistics.set_query_chart_data(true);

    QByteArray pack = ProtocolHelper::getInstance()->createPackage(CMD_QUERY_STATISTICS_DATA_BY_CONDITION, requestStatistics.SerializeAsString());
    UserNetWorker::getInstance()->sendPack(pack);
}

void DialogStatistics::slot_query_statistics_data_by_condition_response(const RequestStatisticsResponse &response)
{
    if (!response.status()) return;

    // 前言信息
    emit mJsContextPreface->sgl_change_preface_info(response.preface().data());

    emit mSurveyContext->sgl_add_remote_trajectory_entitys(response.dt().data());
    emit mSurveyContext->sgl_add_remote_trajectory_entitys(response.auv().data());
    emit mSurveyContext->sgl_add_remote_trajectory_entitys(response.hov().data());
    emit mSurveyContext->sgl_add_remote_trajectory_entitys(response.ship().data());

    emit mSurveyContext->sgl_add_remote_point_entitys(response.errorpoint().data());

    emit mJsContextChartYear->sgl_load_year_curve_chart(response.chart_data().data());
    emit mJsContextChartChecked->sgl_load_check_pie_chart(response.chart_data().data());
    emit mJsContextChartPriority->sgl_load_priority_pie_chart(response.chart_data().data());
}
