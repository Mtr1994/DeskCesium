﻿#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Channel/jscontext.h"
#include "Public/appsignal.h"
#include "gdal_priv.h"
#include "gdal_utils.h"
#include "Dialog/dialogabout.h"
#include "Message/messagewidget.h"
#include "Dialog/dialogsetting.h"
#include "Public/softconfig.h"

#include <QScreen>
#include <QWebEngineSettings>
#include <QWebChannel>
#include <QWebEngineScript>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QFileDialog>
#include <QRgb>
#include <QLinearGradient>
#include <QPainter>
#include <thread>
#include <QImage>

// test
#include <QDebug>

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
    connect(AppSignal::getInstance(), &AppSignal::sgl_change_mouse_over_pick, this, &MainWindow::slot_change_mouse_over_pick);
    connect(AppSignal::getInstance(), &AppSignal::sgl_search_local_altitude, this, &MainWindow::slot_search_local_altitude);
    connect(AppSignal::getInstance(), &AppSignal::sgl_thread_report_system_error, this, &MainWindow::slot_thread_report_system_error, Qt::QueuedConnection);

    ui->widgetCesium->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    ui->widgetCesium->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    ui->widgetCesium->page()->load(QUrl(QString("%1/../Resource/html/index.html").arg(QApplication::applicationDirPath())).toString());

    // 菜单
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::slot_open_files);
    connect(ui->actionExit, &QAction::triggered, this, [this]{ this->close();});

    connect(ui->actionSetting, &QAction::triggered, this, [this]{ DialogSetting dialog(this); dialog.exec(); });

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

    if(poDataset == nullptr)
    {
        emit AppSignal::getInstance()->sgl_thread_report_system_error("栅格图片异常");
        return;
    }
    int xLength = 0, yLength = 0;
    xLength = poDataset->GetRasterXSize();
    yLength = poDataset->GetRasterYSize();

    double adfGeoTransform[6];
    if(poDataset->GetGeoTransform(adfGeoTransform) != CE_None )
    {
        GDALClose((GDALDatasetH)poDataset);
        return;
    }

    double longitudeFrom = adfGeoTransform[0];
    double longitudeEnd = adfGeoTransform[0] + xLength * adfGeoTransform[1] + yLength * adfGeoTransform[2];
    double latitudeFrom = adfGeoTransform[3] + xLength * adfGeoTransform[4] + yLength * adfGeoTransform[5];
    double latitudeEnd = adfGeoTransform[3];

    // 投影坐标
    OGRSpatialReference spatialReference;
    spatialReference.importFromWkt(poDataset->GetProjectionRef());

    // 地理坐标
    OGRSpatialReference *pLonLat = spatialReference.CloneGeogCS();
    OGRCoordinateTransformation *mLonLat2XY = OGRCreateCoordinateTransformation(&spatialReference, pLonLat);

    if (!mLonLat2XY->Transform(1, &longitudeFrom, &latitudeFrom) || !mLonLat2XY->Transform(1, &longitudeEnd, &latitudeEnd))
    {
        emit AppSignal::getInstance()->sgl_thread_report_system_error("无法解析的经纬度信息");
        GDALClose((GDALDatasetH)poDataset);
        return;
    }

    QFileInfo info(path);
    QImage image(path);

    QDir dir = QApplication::applicationDirPath();
    if (!dir.exists("caches"))
    {
        bool status = dir.mkdir("caches");
        if (!status)
        {
            emit AppSignal::getInstance()->sgl_thread_report_system_error("创建缓存文件夹失败");
            return;
        }
    }

    QString suffixTime = QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz");
    bool status = image.save(QString("caches/%1-%2.png").arg(info.baseName(), suffixTime));
    if (!status)
    {
        emit AppSignal::getInstance()->sgl_thread_report_system_error("生成图片失败");
        return;
    }

    QString args = QString("%1,%2,%3,%4,%5").arg(QString::number(longitudeFrom, 'f', 6),
                                                 QString::number(latitudeFrom, 'f', 6),
                                                 QString::number(longitudeEnd, 'f', 6),
                                                 QString::number(latitudeEnd, 'f', 6),
                                                 QString("%1/caches/%2-%3.png").arg(dir.absolutePath(), info.baseName(), suffixTime));

    emit mJsContext->sgl_add_entity("tif", args);

    GDALClose((GDALDatasetH)poDataset);
}

void MainWindow::slot_add_grd_entity(const QString &path)
{
    auto func = [path, this]()
    {
        GDALDataset *poDataset;
        GDALAllRegister();
        poDataset = (GDALDataset *) GDALOpen(path.toStdString().data(), GA_ReadOnly);

        if(poDataset == nullptr)
        {
            emit AppSignal::getInstance()->sgl_thread_report_system_error("打开栅格文件失败");
            return;
        }
        int xLength = 0, yLength = 0;
        xLength = poDataset->GetRasterXSize();
        yLength = poDataset->GetRasterYSize();

        double adfGeoTransform[6];
        if(poDataset->GetGeoTransform(adfGeoTransform) != CE_None )
        {
            GDALClose((GDALDatasetH)poDataset);
            return;
        }

        double longitudeFrom = adfGeoTransform[0];
        double longitudeEnd = adfGeoTransform[0] + xLength * adfGeoTransform[1] + yLength * adfGeoTransform[2];
        double latitudeFrom = adfGeoTransform[3] + xLength * adfGeoTransform[4] + yLength * adfGeoTransform[5];
        double latitudeEnd = adfGeoTransform[3];

        // 投影坐标
        OGRSpatialReference spatialReference;
        OGRErr error = spatialReference.importFromWkt(poDataset->GetProjectionRef());
        if (OGRERR_NONE == error)
        {
            // 地理坐标
            OGRSpatialReference *pLonLat = spatialReference.CloneGeogCS();
            OGRCoordinateTransformation *mLonLat2XY = OGRCreateCoordinateTransformation(&spatialReference, pLonLat);

            if (!mLonLat2XY->Transform(1, &longitudeFrom, &latitudeFrom) || !mLonLat2XY->Transform(1, &longitudeEnd, &latitudeEnd))
            {
                emit AppSignal::getInstance()->sgl_thread_report_system_error("无法解析的经纬度信息");
                GDALClose((GDALDatasetH)poDataset);
                return;
            }
        }

        GDALRasterBand *poBand;
        int nBlockXSize, nBlockYSize;
        int nGotMin, nGotMax;
        double adfMinMax[2] = {0};
        poBand = poDataset->GetRasterBand(1);

        poBand->GetBlockSize( &nBlockXSize, &nBlockYSize);
        GDALDataType cellType = poBand->GetRasterDataType();
        int cellSize = GDALGetDataTypeSizeBytes(cellType);
        adfMinMax[0] = poBand->GetMinimum( &nGotMin );
        adfMinMax[1] = poBand->GetMaximum( &nGotMax );

        if( !( nGotMin && nGotMax ))
        {
           GDALComputeRasterMinMax( (GDALRasterBandH)poBand, TRUE, adfMinMax );
        }

        double range = adfMinMax[1] - adfMinMax[0];
        char *pafScanline = new char[cellSize * yLength * xLength];
        CPLErr err = poBand->RasterIO(GF_Read, 0, 0, xLength, yLength, pafScanline, xLength, yLength, cellType, 0, 0);
        if (err != CE_None)
        {
            emit AppSignal::getInstance()->sgl_thread_report_system_error("读取栅格文件数据异常");
            return;
        }

        double emptyVlaue = poBand->GetNoDataValue();

        QLinearGradient linearGrad({0, 0}, {100, 0});
        linearGrad.setColorAt(0, QColor(0, 0, 255));
        linearGrad.setColorAt(0.25, QColor(0, 255, 255));
        linearGrad.setColorAt(0.5, QColor(0, 255, 0));
        linearGrad.setColorAt(0.75, QColor(255, 255, 0));
        linearGrad.setColorAt(1, QColor(255, 0, 0));

        QImage tempImage(100, 1, QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&tempImage);
        painter.setBrush(QBrush(linearGrad));
        painter.setPen(Qt::NoPen);
        painter.drawRect(tempImage.rect());

        QImage image(xLength, yLength, QImage::Format_ARGB32_Premultiplied);
        QVector<GrdPoint*> vectorPoint;
        for (int m = 0 ; m < xLength; m++)
        {
            for (int n = 0; n < yLength; n++)
            {
                double Xgeo = adfGeoTransform[0] + m * adfGeoTransform[1] + n * adfGeoTransform[2];
                double Ygeo = adfGeoTransform[3] + m * adfGeoTransform[4] + n * adfGeoTransform[5];

                double value = getValue(pafScanline, m + xLength * n, cellSize, cellType);
                if ((value == emptyVlaue) || (value < adfMinMax[0]) || (value > adfMinMax[1]))
                {
                    GrdPoint* point = new GrdPoint;
                    point->longitude = Xgeo;
                    point->latitude = Ygeo;
                    point->altitude = 10000;
                    vectorPoint.append(point);

                    image.setPixel(m, n, qRgba(0, 0, 0, 0));
                    continue;
                }

                GrdPoint* point = new GrdPoint;
                point->longitude = Xgeo;
                point->latitude = Ygeo;
                point->altitude = value;
                vectorPoint.append(point);

                value = floor((value - adfMinMax[0]) * 100 / range);
                QColor color = tempImage.pixelColor((value >= 100 ? 99 : value), 0);
                image.setPixelColor(m, n, qRgba(color.redF() * 255, color.greenF() * 255, color.blueF() * 255, 255));
            }
        }

        delete [] pafScanline;

        GDALClose((GDALDatasetH)poDataset);

        QDir dir = QApplication::applicationDirPath();
        if (!dir.exists("caches"))
        {
            bool status = dir.mkdir("caches");
            if (!status)
            {
                for (auto &point : vectorPoint)
                {
                    delete point;
                }

                emit AppSignal::getInstance()->sgl_thread_report_system_error("创建缓存文件夹失败");
                return;
            }
        }

        QFileInfo info(path);
        QString suffixTime = QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz");
        bool status = image.save(QString("caches/%1-%2.png").arg(info.baseName(), suffixTime));
        if (!status)
        {
            for (auto &point : vectorPoint)
            {
                delete point;
            }
            emit AppSignal::getInstance()->sgl_thread_report_system_error("生成图片失败");
            return;
        }

        QString args = QString("%1,%2,%3,%4,%5").arg(QString::number(longitudeFrom, 'f', 6),
                                                     QString::number(latitudeFrom, 'f', 6),
                                                     QString::number(longitudeEnd, 'f', 6),
                                                     QString::number(latitudeEnd, 'f', 6),
                                                     QString("%1/caches/%2-%3.png").arg(dir.absolutePath(), info.baseName(), suffixTime));


        // 缓存坐标
        QString rect = QString("%1,%2,%3,%4,%5").arg(QString::number(adfGeoTransform[0], 'f', 6),
                QString::number(adfGeoTransform[0] + (xLength - 1) * adfGeoTransform[1] + (yLength - 1) * adfGeoTransform[2], 'f', 6),
                QString::number(adfGeoTransform[3], 'f' ,6),
                QString::number(adfGeoTransform[3] + (xLength - 1) * adfGeoTransform[4] + (yLength - 1) * adfGeoTransform[5], 'f', 6),
                QString("%1/caches/%2-%3.png").arg(dir.absolutePath(), info.baseName(), suffixTime));

        mMapPositions.insert(rect, vectorPoint);

        emit mJsContext->sgl_add_entity("grd", args);
    };

    std::thread th(func);
    th.detach();
}

void MainWindow::slot_change_entity_status(const QString &type, const QString &name, bool visible, const QString &parentid)
{
    emit mJsContext->sgl_change_entity_visible(type, name, visible, parentid);
}

void MainWindow::slot_delete_cesium_data_source(const QString &type, const QString &name)
{
    if (type == "grd")
    {
        auto func = [this, name]
        {
            QMap<QString, QVector<GrdPoint*>>::const_iterator iter = mMapPositions.constBegin();
            while (iter != mMapPositions.constEnd())
            {
                auto list = iter.key().split(',');
                if (name != list[4]) continue;

                QVector<GrdPoint*> vector = mMapPositions.take(iter.key());
                for (auto &point : vector)
                {
                    delete point;
                }
                break;
            }
        };

        // 清理内存
        std::thread th(func);
        th.detach();
    }

    emit mJsContext->sgl_delete_entity(type, name);
}

void MainWindow::slot_fly_to_entity(const QString &type, const QString &id, const QString &parentId)
{
    emit mJsContext->sgl_fly_to_entity(type, id, parentId);
}

void MainWindow::slot_change_mouse_over_pick(bool open)
{
    emit mJsContext->sgl_change_mouse_over_status(open);
}

void MainWindow::slot_search_local_altitude(double longitude, double latitude)
{
    if ((longitude == mCurrentLongitude) && (latitude == mCurrentLatitude)) return;
    mCurrentLongitude = longitude;
    mCurrentLatitude = latitude;

    auto func = [longitude, latitude, this]
    {
        double altitude = 10000.00;
        QMap<QString, QVector<GrdPoint*>>::const_iterator iter = mMapPositions.constBegin();
        while (iter != mMapPositions.constEnd())
        {
            auto list = iter.key().split(',');
            if ((longitude < list.at(0).toDouble()) || (longitude > list.at(1).toDouble()) || (latitude < list.at(3).toDouble()) || (latitude > list.at(2).toDouble()))
            {
                iter++;
                continue;
            }

            double minRange = 500.00;
            QVector<GrdPoint*> vector = iter.value();
            for (auto &point : vector)
            {
                double tmpRange = sqrt(pow(abs(point->longitude - longitude), 2) + pow(abs(point->latitude - latitude), 2));
                if (minRange > tmpRange)
                {
                    minRange = tmpRange;
                    altitude = point->altitude;
                }
            }
            break;
        }
        emit mJsContext->sgl_search_mouse_over_altitude(longitude, latitude, altitude != 10000.00, altitude);
    };

    std::thread th(func);
    th.detach();
}

void MainWindow::slot_thread_report_system_error(const QString &msg)
{
    MessageWidget *message = new MessageWidget(MessageWidget::M_Info, MessageWidget::P_Top_Center, ui->widgetBase);
    message->showMessage(msg);
}

void MainWindow::slot_open_files()
{
    QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    QString filter;
    QString path = QFileDialog::getOpenFileName(this, "选择文件", desktop, "支持的文件(*.kml *.kmz *.tif *.grd);; kml 文件(*.kml);; kmz 文件(*.kmz);; tiff (*.tif);; grd (*.grd)", &filter);
    if (path.isEmpty()) return;
    QFileInfo info(path);
    QString suffix = info.suffix();
    if (suffix == "tif") { slot_add_tiff_entity(path); }
    else if (suffix == "grd") { slot_add_grd_entity(path); }
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

double MainWindow::getValue(char *array, int index, int cellsize, int celltype)
{
    switch (celltype) {
    case 1:
        char value1;
        memcpy(&value1, array + index * cellsize, cellsize);
        return value1;
    case 2:
        unsigned short value2;
        memcpy(&value2, array + index * cellsize, cellsize);
        return value2;
    case 3:
        short value3;
        memcpy(&value3, array + index * cellsize, cellsize);
        return value3;
    case 4:
        unsigned int value4;
        memcpy(&value4, array + index * cellsize, cellsize);
        return value4;
    case 5:
        int value5;
        memcpy(&value5, array + index * cellsize, cellsize);
        return value5;
    case 6:
        float value6;
        memcpy(&value6, array + index * cellsize, cellsize);
        return value6;
    case 7:
        double value7;
        memcpy(&value7, array + index * cellsize, cellsize);
        return value7;
    default:
        return 0;
    }
}
