#include "mainwindow.h"
#include "Public/softconfig.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    // 加载样式
    qApp->setStyleSheet("file:///:/Resource/qss/style.qss");

    /// 初始化配置
    bool status = SoftConfig::getInstance()->init();
    if (!status) return 0;

    MainWindow w;
    w.show();

    return a.exec();
}
