#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Channel/jscontext.h"
#include "Public/appsignal.h"
#include "gdal_priv.h"
#include "Dialog/dialogabout.h"

#include <QScreen>
#include <QWebEngineSettings>
#include <QWebChannel>
#include <QWebEngineScript>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QFileDialog>

// test
#include <QDebug>
#include <QImage>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init();

    setWindowTitle("Cesium Earth");
}

MainWindow::~MainWindow()
{
    delete mJsContext;

    // 清理 tif 缓存
    QDir dir = QApplication::applicationDirPath();
    if (!dir.exists("caches")) return;
    removeFolderContent(QString("%1/caches").arg(dir.absolutePath()));

    delete ui;
}

void MainWindow::init()
{
    QScreen *screen = QGuiApplication::screens().at(0);
    float width = 1024;
    float height = 640;
    if (nullptr != screen)
    {
        QRect rect = screen->availableGeometry();
        width = rect.width() * 0.64 < 1024 ? 1024 : rect.width() * 0.64;
        height = rect.height() * 0.64 < 640 ? 640 : rect.height() * 0.64;
    }

    resize(width, height);

    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 72);

    mJsContext = new JsContext(this);
    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject("context", mJsContext);
    ui->widgetCesium->page()->setWebChannel(channel);

    connect(AppSignal::getInstance(), &AppSignal::sgl_add_kml_entity, this, &MainWindow::slot_add_kml_entity);
    connect(AppSignal::getInstance(), &AppSignal::sgl_add_kmz_entity, this, &MainWindow::slot_add_kmz_entity);
    connect(AppSignal::getInstance(), &AppSignal::sgl_add_tiff_entity, this, &MainWindow::slot_add_tiff_entity);
    connect(AppSignal::getInstance(), &AppSignal::sgl_change_entity_status, this, &MainWindow::slot_change_entity_status);
    connect(AppSignal::getInstance(), &AppSignal::sgl_delete_cesium_data_source, this, &MainWindow::slot_delete_cesium_data_source);
    connect(AppSignal::getInstance(), &AppSignal::sgl_fly_to_entity, this, &MainWindow::slot_fly_to_entity);

    ui->widgetCesium->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    ui->widgetCesium->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    ui->widgetCesium->page()->load(QUrl(QString("%1/../Resource/html/index.html").arg(QApplication::applicationDirPath())).toString());

    // 菜单
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::slot_open_files);
    connect(ui->actionExit, &QAction::triggered, this, []{ exit(0);});

    connect(ui->actionMeasureLine, &QAction::triggered, this, &MainWindow::slot_start_measure_line);
    connect(ui->actionMeasurePolygn, &QAction::triggered, this, &MainWindow::slot_start_measure_polygn);

    // 版本信息
    connect(ui->actionVersion, &QAction::triggered, this, [this]{ DialogAbout dialog(this); dialog.exec(); });
}

void MainWindow::slot_add_kml_entity(const QString &path)
{
    emit mJsContext->sgl_add_entity("kml", path);
}

void MainWindow::slot_add_kmz_entity(const QString &path)
{
    emit mJsContext->sgl_add_entity("kmz", path);
}

void MainWindow::slot_add_tiff_entity(const QString &path)
{
    GDALDataset *poDataset;
    GDALAllRegister();
    poDataset = (GDALDataset *) GDALOpen(path.toStdString().data(), GA_ReadOnly);

    if(poDataset == nullptr) return;
    int xLength = 0, yLength = 0;
    xLength = poDataset->GetRasterXSize();
    yLength = poDataset->GetRasterYSize();

    double adfGeoTransform[6];
    if(poDataset->GetGeoTransform(adfGeoTransform) != CE_None ) return;

    double longitudeFrom = adfGeoTransform[0];
    double longitudeEnd = adfGeoTransform[0] + xLength * adfGeoTransform[1] + yLength * adfGeoTransform[2];
    double latitudeFrom = adfGeoTransform[3] + xLength * adfGeoTransform[4] + yLength * adfGeoTransform[5];
    double latitudeEnd = adfGeoTransform[3];

    QFileInfo info(path);
    QImage image(path);

    QDir dir = QApplication::applicationDirPath();
    if (!dir.exists("caches"))
    {
        bool status = dir.mkdir("caches");
        if (!status)
        {
            qDebug() << "create dir failed ";
            return;
        }
    }

    QString suffixTime = QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz");
    bool status = image.save(QString("caches/%1-%2.png").arg(info.baseName(), suffixTime));
    if (!status)
    {
        qDebug() << "create png file failed ";
        return;
    }

    QString args = QString("%1,%2,%3,%4,%5").arg(QString::number(longitudeFrom, 'f', 6),
                                                 QString::number(latitudeFrom, 'f', 6),
                                                 QString::number(longitudeEnd, 'f', 6),
                                                 QString::number(latitudeEnd, 'f', 6),
                                                 QString("%1/caches/%2-%3.png").arg(dir.absolutePath(), info.baseName(), suffixTime));

    emit mJsContext->sgl_add_entity("tif", args);
}

void MainWindow::slot_change_entity_status(const QString &type, const QString &name, bool visible, const QString &parentid)
{
    emit mJsContext->sgl_change_entity_visible(type, name, visible, parentid);
}

void MainWindow::slot_delete_cesium_data_source(const QString &type, const QString &name)
{
    emit mJsContext->sgl_delete_entity(type, name);
}

void MainWindow::slot_fly_to_entity(const QString &type, const QString &id, const QString &parentId)
{
    emit mJsContext->sgl_fly_to_entity(type, id, parentId);
}

void MainWindow::slot_open_files()
{
    QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    QString filter;
    QString path = QFileDialog::getOpenFileName(this, "选择文件", desktop, "支持的文件(*.kml *.kmz *.tif);; kml 文件(*.kml);; kmz 文件(*.kmz);; tiff (*.tif)", &filter);
    if (path.isEmpty()) return;
    QFileInfo info(path);
    QString suffix = info.suffix();
    if (suffix == "tif") { slot_add_tiff_entity(path); }
    else emit mJsContext->sgl_add_entity(suffix, path);
}

void MainWindow::slot_start_measure_line()
{
    QString id = QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz");
    emit mJsContext->sgl_start_measure("mla", id);
}

void MainWindow::slot_start_measure_polygn()
{
    QString id = QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz");
    emit mJsContext->sgl_start_measure("mpa", id);
}

bool MainWindow::removeFolderContent(const QString &folderDir)
{
    QDir dir(folderDir);
    QFileInfoList fileList;
    QFileInfo curFile;
    if(!dir.exists())  {return false;}//文件不存，则返回false
    fileList=dir.entryInfoList(QDir::Dirs|QDir::Files
                               |QDir::Readable|QDir::Writable
                               |QDir::Hidden|QDir::NoDotAndDotDot
                               ,QDir::Name);
    while(fileList.size()>0)
    {
        int infoNum=fileList.size();
        for(int i=infoNum-1;i>=0;i--)
        {
            curFile=fileList[i];
            if(curFile.isFile())//如果是文件，删除文件
            {
                QFile fileTemp(curFile.filePath());
                fileTemp.remove();
                fileList.removeAt(i);
            }
            if(curFile.isDir())//如果是文件夹
            {
                QDir dirTemp(curFile.filePath());
                QFileInfoList fileList1=dirTemp.entryInfoList(QDir::Dirs|QDir::Files
                                                              |QDir::Readable|QDir::Writable
                                                              |QDir::Hidden|QDir::NoDotAndDotDot
                                                              ,QDir::Name);
                if(fileList1.size()==0)//下层没有文件或文件夹
                {
                    dirTemp.rmdir(".");
                    fileList.removeAt(i);
                }
                else//下层有文件夹或文件
                {
                    for(int j=0;j<fileList1.size();j++)
                    {
                        if(!(fileList.contains(fileList1[j])))
                            fileList.append(fileList1[j]);
                    }
                }
            }
        }
    }
    return true;
}

