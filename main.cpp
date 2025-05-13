#include "mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置应用程序信息
    QApplication::setApplicationName("停车场预约管理系统");
    QApplication::setOrganizationName("QtDevelopment");
    
    qDebug() << "当前工作目录:" << QDir::currentPath();
    
    // 检查数据库驱动是否可用
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        qDebug() << "SQLite 数据库驱动不可用!";
        return -1;
    }
    
    MainWindow w;
    w.show();
    
    return a.exec();
}
