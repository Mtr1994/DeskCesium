#include "widgetsearch.h"
#include "ui_widgetsearch.h"
#include "Dialog/dialogsearch.h"

// test
#include <QDebug>

WidgetSearch::WidgetSearch(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetSearch)
{
    ui->setupUi(this);

    init();
}

WidgetSearch::~WidgetSearch()
{
    delete ui;
}

void WidgetSearch::init()
{
    connect(ui->btnSearch, &QPushButton::clicked, this, &WidgetSearch::slot_btn_search_click);
    connect(ui->btnComplexSearch, &QPushButton::clicked, this, &WidgetSearch::slot_btn_complex_search_click);
}

void WidgetSearch::slot_btn_search_click()
{
    qDebug() << "slot_btn_search_click " << "这里需要判断参数是否合法";

    DialogSearch dialog("year:2021", this);
    dialog.exec();
}

void WidgetSearch::slot_btn_complex_search_click()
{
    DialogSearch dialog("", this);
    dialog.exec();
}
