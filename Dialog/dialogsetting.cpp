#include "dialogsetting.h"
#include "ui_dialogsetting.h"

#include <QRegExp>
#include <QRegExpValidator>

#include "Public/treeitemdelegate.h"
#include "Public/softconfig.h"
#include "Public/appsignal.h"

DialogSetting::DialogSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSetting)
{
    ui->setupUi(this);

    ui->tabSettingWidget->tabBar()->hide();

    initMenuItems();
    initTabBase();

    this->setWindowTitle("首选项");
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

    resize(this->nativeParentWidget()->width() / 2.0, this->nativeParentWidget()->height() * 0.4);
}

DialogSetting::~DialogSetting()
{
    delete ui;
}

void DialogSetting::initMenuItems()
{
    mModelMenuItems = new QStandardItemModel(this);
    QStandardItem* base = new QStandardItem("基础设置");
    mModelMenuItems->appendRow(base);

    ui->treeMenuItems->setModel(mModelMenuItems);
    ui->treeMenuItems->setItemDelegate(new TreeItemDelegate);

    ui->treeMenuItems->setCurrentIndex(mModelMenuItems->index(0, 0));
    ui->treeMenuItems->expandAll();
}

void DialogSetting::initTabBase()
{
    mListInit.append(DialogSetting::Tab_Base);
    bool isOpen = SoftConfig::getInstance()->getValue("Base", "openMouseOver").toUInt();
    ui->cbMouseOver->setChecked(isOpen);
}

void DialogSetting::on_btnCancel_clicked()
{
    done(0);
}

/**
 * @brief DialogSetting::on_btnApply_clicked
 * 只保存当前展示的界面的数据
 */

void DialogSetting::on_btnApply_clicked()
{
    if (ui->tabSettingWidget->currentIndex() == Tab_Base)
    {
        bool isOpen = SoftConfig::getInstance()->getValue("Base", "openMouseOver").toUInt();
        if (isOpen == ui->cbMouseOver->isChecked()) return;

        SoftConfig::getInstance()->setValue("Base", "openMouseOver", QString::number(ui->cbMouseOver->isChecked()));

       emit AppSignal::getInstance()->sgl_change_mouse_over_pick(!isOpen);
    }
}

void DialogSetting::on_btnOk_clicked()
{
    on_btnApply_clicked();
    done(1);
}

void DialogSetting::showError(const QString &error)
{
    ui->lbError->setText("<i><font color='#e94918'>" + error + "</i>");
}

/**
 * @brief DialogSetting::on_treeMenuItems_pressed
 * @param index
 * 改变左侧选中的菜单项
 */

void DialogSetting::on_treeMenuItems_pressed(const QModelIndex &index)
{
    if(!index.isValid()) return;
    QString menu = mModelMenuItems->itemFromIndex(index)->text();

    if (menu == "基础设置")
    {
        ui->tabSettingWidget->setCurrentIndex(Tab_Base);
    }
}
