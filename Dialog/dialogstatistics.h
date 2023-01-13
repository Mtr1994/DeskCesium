#ifndef DIALOGSTATISTICS_H
#define DIALOGSTATISTICS_H

#include "Proto/sidescansource.pb.h"

#include <QDialog>

namespace Ui {
class DialogStatistics;
}

class SurveyContext;
class ChartContext;
class PrefaceJsContext;
class DialogStatistics : public QDialog
{
    Q_OBJECT

public:
    explicit DialogStatistics(QWidget *parent = nullptr);
    ~DialogStatistics();

private:
    void init();

private:
    void requestStatisticsData();

private slots:
    // 查询统计数据结果
    void slot_query_statistics_data_by_condition_response(const RequestStatisticsResponse &response);

private:
    Ui::DialogStatistics *ui;

    // 通信类
    SurveyContext *mSurveyContext = nullptr;

    // 图表专属通信类
    ChartContext *mJsContextChartYear = nullptr;

    // 图表专属通信类
    ChartContext *mJsContextChartPriority = nullptr;

    // 图表专属通信类
    ChartContext *mJsContextChartChecked = nullptr;

    // 前言通信类
    PrefaceJsContext *mJsContextPreface = nullptr;

    uint16_t mNumberInitWebView = 0;
};

#endif // DIALOGSTATISTICS_H
