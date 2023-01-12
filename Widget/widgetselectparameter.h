#ifndef WIDGETSELECTPARAMETER_H
#define WIDGETSELECTPARAMETER_H

#include "Proto/sidescansource.pb.h"

#include <QWidget>

namespace Ui {
class WidgetSelectParameter;
}

class WidgetSelectParameter : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetSelectParameter(QWidget *parent = nullptr);
    ~WidgetSelectParameter();

    void requestSelectParameter();

signals:
    // 修改检索条件
    void sgl_modify_search_parameter(const QString &target, const QString &value, bool append);

private:
    void init();

private slots:
    void slot_query_search_filter_parameter_response(const SearchFilterParamterList &response);

private:
    Ui::WidgetSelectParameter *ui;
};

#endif // WIDGETSELECTPARAMETER_H
