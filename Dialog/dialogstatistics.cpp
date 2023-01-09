#include "dialogstatistics.h"
#include "ui_dialogstatistics.h"
#include "Channel/jscontext.h"
#include "Channel/chartcontext.h"

#include <QWebEngineSettings>
#include <QWebChannel>

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

    setMinimumHeight(nativeParentWidget()->height());
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    mJsContext = new JsContext(this);
    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject("context", mJsContext);
    ui->widgetStatisticsCesium->page()->setWebChannel(channel);

    // Cesium 组件
    ui->widgetStatisticsCesium->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    ui->widgetStatisticsCesium->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    ui->widgetStatisticsCesium->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    ui->widgetStatisticsCesium->page()->load(QUrl(QString("%1/../Resource/html/indexStatisticsCesium.html").arg(QApplication::applicationDirPath())).toString());
    ui->widgetStatisticsCesium->page()->setBackgroundColor(QColor(0, 0, 0));


    mJsContextChartYear = new ChartContext(this);
    QWebChannel *channelChartYear = new QWebChannel(this);
    channelChartYear->registerObject("context", mJsContextChartYear);

    // 折线图 年份
    ui->widgetChartCurveYear->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    ui->widgetChartCurveYear->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    ui->widgetChartCurveYear->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    ui->widgetChartCurveYear->page()->setWebChannel(channelChartYear);
    ui->widgetChartCurveYear->page()->load(QUrl(QString("%1/../Resource/html/indexStatisticsChart.html").arg(QApplication::applicationDirPath())).toString());
    ui->widgetChartCurveYear->page()->setBackgroundColor(QColor(255, 255, 255));

    connect(mJsContextChartYear, &ChartContext::sgl_web_view_init_finish, this, [this]{ emit mJsContextChartYear->sgl_load_year_curve_chart("{year: ['2021', '2022'], value: [46, 78]}"); });

    // 饼图 查证
    mJsContextChartChecked = new ChartContext(this);
    QWebChannel *channelChartChecked = new QWebChannel(this);
    channelChartChecked->registerObject("context", mJsContextChartChecked);
    ui->widgetChartPieChecked->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    ui->widgetChartPieChecked->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    ui->widgetChartPieChecked->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    ui->widgetChartPieChecked->page()->setWebChannel(channelChartChecked);
    ui->widgetChartPieChecked->page()->load(QUrl(QString("%1/../Resource/html/indexStatisticsChart.html").arg(QApplication::applicationDirPath())).toString());
    ui->widgetChartPieChecked->page()->setBackgroundColor(QColor(255, 255, 255));

    connect(mJsContextChartChecked, &ChartContext::sgl_web_view_init_finish, this, [this]{ emit mJsContextChartChecked->sgl_load_check_pie_chart("[{value: 70, name: '已查证'}, {value: 30, name: '未查证'}]"); });

    // 饼图 优先级
    mJsContextChartPriority = new ChartContext(this);
    QWebChannel *channelChartPriority = new QWebChannel(this);
    channelChartPriority->registerObject("context", mJsContextChartPriority);
    ui->widgetChartPiePriority->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    ui->widgetChartPiePriority->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    ui->widgetChartPiePriority->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    ui->widgetChartPiePriority->page()->setWebChannel(channelChartPriority);
    ui->widgetChartPiePriority->page()->load(QUrl(QString("%1/../Resource/html/indexStatisticsChart.html").arg(QApplication::applicationDirPath())).toString());
    ui->widgetChartPiePriority->page()->setBackgroundColor(QColor(255, 255, 255));

//    ////// 网页调试部分，发布时请注释此段代码 S
//    QWebEngineView *debugPage = new QWebEngineView;
//    ui->widgetChartPiePriority->page()->setDevToolsPage(debugPage->page());
//    ui->widgetChartPiePriority->page()->triggerAction(QWebEnginePage::WebAction::InspectElement);
//    debugPage->show();
//    ////// 网页调试部分，发布时请注释此段代码 E

    connect(mJsContextChartPriority, &ChartContext::sgl_web_view_init_finish, this, [this]{ emit mJsContextChartPriority->sgl_load_priority_pie_chart("[{value: 24, name: '优先级1'}, {value: 46, name: '优先级2'}, {value: 30, name: '优先级3'}]"); });
}
