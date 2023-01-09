#ifndef DIALOGSTATISTICS_H
#define DIALOGSTATISTICS_H

#include <QDialog>

namespace Ui {
class DialogStatistics;
}

class JsContext;
class ChartContext;
class DialogStatistics : public QDialog
{
    Q_OBJECT

public:
    explicit DialogStatistics(QWidget *parent = nullptr);
    ~DialogStatistics();

private:
    void init();

private:
    Ui::DialogStatistics *ui;

    // 通信类
    JsContext *mJsContext = nullptr;

    // 图表专属通信类
    ChartContext *mJsContextChartYear = nullptr;

    // 图表专属通信类
    ChartContext *mJsContextChartPriority = nullptr;

    // 图表专属通信类
    ChartContext *mJsContextChartChecked = nullptr;
};

#endif // DIALOGSTATISTICS_H
