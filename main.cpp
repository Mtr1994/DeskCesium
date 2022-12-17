#include "mainwindow.h"
#include "Public/appconfig.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QApplication a(argc, argv);

    // 加载样式
    qApp->setStyleSheet("file:///:/Resource/qss/style.qss");

    /// 初始化配置
    AppConfig::getInstance()->init();

    MainWindow w;
    w.show();

    return a.exec();
}
