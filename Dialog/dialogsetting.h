#ifndef DIALOGSETTING_H
#define DIALOGSETTING_H

#include <QDialog>
#include <QStandardItemModel>
#include <QList>

namespace Ui {
class DialogSetting;
}

class DialogSetting : public QDialog
{
    Q_OBJECT

public:
    enum TabOrder{ Tab_Base };
    explicit DialogSetting(QWidget *parent = nullptr);
    ~DialogSetting();

    void initMenuItems();

    void initTabBase();

private slots:
    void on_btnCancel_clicked();

    void on_btnApply_clicked();

    void on_btnOk_clicked();

    void on_treeMenuItems_pressed(const QModelIndex &index);

private:
    void showError(const QString& error);

private:
    Ui::DialogSetting *ui;
    QStandardItemModel *mModelMenuItems = nullptr;

    /**
     * @brief mListInit
     * 记录已初始化过的 Tab 窗口
     */
    QList<int> mListInit;
};

#endif // DIALOGSETTING_H
