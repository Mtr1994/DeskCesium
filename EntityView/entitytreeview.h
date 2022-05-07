#ifndef ENTITYTREEVIEW_H
#define ENTITYTREEVIEW_H

#include <QObject>
#include <QTreeView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QStandardItemModel>

class EntityTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit EntityTreeView(QWidget *parent = nullptr);

    void init();

signals:

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

    // handle add entity result
    void slot_add_entity_finish(const QString &type, const QString &arg);

    // handle delete entity result
    void slot_delete_entity_finish(const QString &arg);

private:
    QStandardItemModel *mEntityModel = nullptr;
};

#endif // ENTITYTREEVIEW_H
