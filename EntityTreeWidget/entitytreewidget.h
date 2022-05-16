#ifndef ENTITYTREEWIDGET_H
#define ENTITYTREEWIDGET_H

#include <QWidget>
#include <QTreeView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QStandardItemModel>

class EntityTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit EntityTreeView(QWidget *parent = nullptr);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);;
};

class EntityTreeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EntityTreeWidget(QWidget *parent = nullptr);

    void init();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void slot_context_menu_request(const QPoint &pos);

private:
    void handleDropFile(const QString &path);

    // handle kml file
    void handleKmlFile(const QString &path);

    // handle kmz file
    void handleKmzFile(const QString &path);

    // handle tif file
    void handleTiffFile(const QString &path);

    // handle grd file
    void handlGrdFile(const QString &path);

    // handle add entity result
    void slot_add_entity_finish(const QString &type, const QString &arg, const QString &list);

    // handle delete entity result
    void slot_delete_entity_finish(const QString &arg);

private:
    // 数据模型
    QStandardItemModel *mEntityModel = nullptr;

    // 视图
    EntityTreeView *mTreeView = nullptr;
};

#endif // ENTITYTREEWIDGET_H
