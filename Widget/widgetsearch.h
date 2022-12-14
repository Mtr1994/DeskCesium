#ifndef WIDGETSEARCH_H
#define WIDGETSEARCH_H

#include <QWidget>

namespace Ui {
class WidgetSearch;
}

class WidgetSearch : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetSearch(QWidget *parent = nullptr);
    ~WidgetSearch();

private:
    void init();

private slots:
    void slot_btn_search_click();
    void slot_btn_complex_search_click();

private:
    Ui::WidgetSearch *ui;

};

#endif // WIDGETSEARCH_H
