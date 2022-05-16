#include "entitytreewidget.h"
#include "Public/appsignal.h"
#include "Public/treeitemdelegate.h"
#include "Message/messagewidget.h"

#include <QHeaderView>
#include <QMimeData>
#include <QFileInfo>
#include <QMenu>
#include <QGridLayout>

EntityTreeView::EntityTreeView(QWidget *parent)
    : QTreeView(parent)
{

}

void EntityTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    QModelIndex index = this->currentIndex();
    if (!index.isValid()) return;
    QStandardItem *item = ((QStandardItemModel*)this->model())->itemFromIndex(index);
    if (nullptr == item) return;

    QString type = item->data().toString();
    QString id = item->data(Qt::UserRole + 2).toString();
    if (id.startsWith("Legend")) return;
    QString parentId = item->data(Qt::UserRole + 3).toString();
    emit AppSignal::getInstance()->sgl_fly_to_entity(type, id, parentId);
}

EntityTreeWidget::EntityTreeWidget(QWidget *parent)
    : QWidget{parent}
{
    init();
}

void EntityTreeWidget::init()
{
    mEntityModel = new QStandardItemModel(this);

    mTreeView = new EntityTreeView(this);
    mTreeView->setModel(mEntityModel);
    mTreeView->setEditTriggers(QTreeView::NoEditTriggers);
    mTreeView->setItemDelegate(new TreeItemDelegate);
    mTreeView->header()->setVisible(false);

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(mTreeView);
    layout->setContentsMargins(0, 0, 0, 0);

    setLayout(layout);

    setAcceptDrops(true);

    connect(AppSignal::getInstance(), &AppSignal::sgl_add_entity_finish, this, &EntityTreeWidget::slot_add_entity_finish);
    connect(AppSignal::getInstance(), &AppSignal::sgl_delete_entity_finish, this, &EntityTreeWidget::slot_delete_entity_finish);

    connect(this, &QTreeView::customContextMenuRequested, this, &EntityTreeWidget::slot_context_menu_request);
}

void EntityTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void EntityTreeWidget::dropEvent(QDropEvent *event)
{
    QString path = event->mimeData()->urls().first().toString().remove("file:///");
    handleDropFile(path);
}

void EntityTreeWidget::slot_context_menu_request(const QPoint &pos)
{
    QModelIndex index = mTreeView->indexAt(pos);
    if (!index.isValid()) return;

    QStandardItem *item = mEntityModel->itemFromIndex(index);
    QString type = item->data().toString();
    QString id = item->data(Qt::UserRole + 2).toString();
    bool visible = item->checkState() == Qt::Checked;

    QMenu menu(this);
    QAction actionShow("显示数据集");
    actionShow.setEnabled(!visible);
    connect(&actionShow, &QAction::triggered, [&item]() { item->setCheckState(Qt::Checked); });
    menu.addAction(&actionShow);
    QAction actionHide("隐藏数据集");
    actionHide.setEnabled(visible);
    connect(&actionHide, &QAction::triggered, [&item]() { item->setCheckState(Qt::Unchecked); });
    menu.addAction(&actionHide);

    QAction actionDel("删除数据集");
    if (nullptr == item->parent())
    {
        connect(&actionDel, &QAction::triggered, this, [id, type] { emit AppSignal::getInstance()->sgl_delete_cesium_data_source(type, id); });
        menu.addAction(&actionDel);
    }

    menu.exec(QCursor::pos());
}

void EntityTreeWidget::handleDropFile(const QString &path)
{
    QFileInfo info(path);
    if (!info.exists()) return;
    QString suffix = info.suffix();
    if (suffix == "kml")
    {
        handleKmlFile(path);
    }
    else if (suffix == "kmz")
    {
        handleKmzFile(path);
    }
    else if (suffix == "tif")
    {
        handleTiffFile(path);
    }
    else if (suffix == "grd")
    {
        handlGrdFile(path);
    }
    else
    {
        MessageWidget *message = new MessageWidget(MessageWidget::M_Info, MessageWidget::P_Top_Center, this->parentWidget()->parentWidget()->parentWidget());
        message->showMessage("未支持的数据类型");
    }
}

void EntityTreeWidget::handleKmlFile(const QString &path)
{
    emit AppSignal::getInstance()->sgl_add_kml_entity(path);
}

void EntityTreeWidget::handleKmzFile(const QString &path)
{
    emit AppSignal::getInstance()->sgl_add_kmz_entity(path);
}

void EntityTreeWidget::handleTiffFile(const QString &path)
{
    emit AppSignal::getInstance()->sgl_add_tiff_entity(path);
}

void EntityTreeWidget::handlGrdFile(const QString &path)
{
    emit AppSignal::getInstance()->sgl_add_grd_entity(path);
}

void EntityTreeWidget::slot_add_entity_finish(const QString &type, const QString &arg, const QString &list)
{
    QString text = arg;
    if (type == "tif")
    {
        QFileInfo info(text);
        text = info.baseName() + ".tif";
    }
    if (type == "grd")
    {
        QFileInfo info(text);
        text = info.baseName() + ".grd";
    }
    else if (type == "mla")
    {
        text = "测距离-" + arg;
    }
    else if (type == "mpa")
    {
        text = "测面积-" + arg;
    }

    QStandardItem *item = new QStandardItem(text);
    item->setData(type);
    item->setData(arg, Qt::UserRole + 2);
    item->setCheckable(true);
    item->setCheckState(Qt::Checked);

    connect(mEntityModel, &QStandardItemModel::itemChanged, this, [](QStandardItem *item)
    {
        bool visible = (item->checkState() == Qt::Checked);
        QString type = item->data().toString();
        QString id = item->data(Qt::UserRole + 2).toString();
        QString parentId = item->data(Qt::UserRole + 3).toString();
        emit AppSignal::getInstance()->sgl_change_entity_status(type, id, visible, parentId);

        if (!item->hasChildren()) return;

        int count = item->rowCount();
        for (int i = 0; i < count; i++)
        {
            QStandardItem *child = item->child(i);
            child->setDragEnabled(false);
            child->setDropEnabled(false);
            if (nullptr == child) continue;
            child->setCheckState(item->checkState());
        }
    });

    mEntityModel->appendRow(item);

    if (type == "kml" || type == "kmz")
    {
        QStringList entities = list.split('#', Qt::SkipEmptyParts);
        for (auto &entity : entities)
        {
            QStringList info = entity.split(',');
            if (info.size() != 2) continue;

            QString name = info[0];
            QString id = info[1];
            // 图例不是实体，无法使用 id 区分
            if (info[0] == "Legend") {
                name = info[0] + "-" + info[1];
                id = name;
            }

            QStandardItem *child = new QStandardItem(name);
            child->setData(type);
            child->setData(id, Qt::UserRole + 2);
            child->setData(arg, Qt::UserRole + 3); // parent id
            child->setCheckable(true);
            child->setCheckState(Qt::Checked);

            item->appendRow(child);
        }
    }
}

void EntityTreeWidget::slot_delete_entity_finish(const QString &arg)
{
    QModelIndexList list = mEntityModel->match(mEntityModel->index(0, 0), Qt::UserRole + 2, arg, -1, Qt::MatchExactly | Qt::MatchRecursive);
    if (list.isEmpty()) return;

    for (auto &index : list)
    {
        mEntityModel->removeRow(index.row());
    }
}
