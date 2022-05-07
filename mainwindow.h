#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class JsContext;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void init();

public slots:
    void slot_add_kml_entity(const QString &path);
    void slot_add_kmz_entity(const QString &path);
    void slot_add_tiff_entity(const QString &path);
    void slot_change_entity_status(const QString &type, const QString &name, bool visible, const QString &parentid = "");
    void slot_delete_cesium_data_source(const QString &type, const QString &name);
    void slot_fly_to_entity(const QString &type, const QString &id, const QString &parentId);

private slots:
    void slot_open_files();
    void slot_start_measure_line();
    void slot_start_measure_polygn();

private:
    bool removeFolderContent(const QString &folderDir);

private:
    Ui::MainWindow *ui;

    QWebEngineView *mWebView = nullptr;

    // 通信类
    JsContext *mJsContext = nullptr;
};
#endif // MAINWINDOW_H
