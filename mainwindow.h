#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QList>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

typedef struct
{
    int width;
    int height;
    int cellSize;
    int cellType;
    double emptyValue;
    double minValue;
    double maxValue;
    double *geoTransform;
    char *data;
    std::string name;
} GrdDataSet;

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
    void slot_add_grd_entity(const QString &path);
    void slot_change_entity_status(const QString &type, const QString &name, bool visible, const QString &parentid = "");
    void slot_delete_cesium_data_source(const QString &type, const QString &name);
    void slot_fly_to_entity(const QString &type, const QString &id, const QString &parentId);
    void slot_change_mouse_over_pick();
    void slot_search_local_altitude(double longitude, double latitude);

    // 接收系统错误信息
    void slot_thread_report_system_error(const QString & msg);

    // 组件初始化完成
    void slot_cesium_init_finish();

private slots:
    void slot_open_files();
    void slot_start_measure_line();
    void slot_start_measure_polygn();

    void slot_import_cruise_data();

private:
    bool removeFolderContent(const QString &folderDir);

    double getValue(char *array, int index, int cellsize, int celltype);

private:
    Ui::MainWindow *ui;

    QWebEngineView *mWebView = nullptr;

    // 通信类
    JsContext *mJsContext = nullptr;

    QList<GrdDataSet> mListGrdData;

    // 当前计算的经纬度位置
    double mCurrentLongitude = 0.0;
    double mCurrentLatitude = 0.0;
};
#endif // MAINWINDOW_H
