#include "dbmanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>

DBManager::DBManager(QObject *parent)
    : QObject(parent)
{
    openDB();
    createTables();
}

DBManager::~DBManager()
{
    closeDB();
}

bool DBManager::openDB()
{
    QString dbPath = QDir::currentPath() + "/parking_system.db";
    qDebug() << "Database path: " << dbPath;
    
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);
    
    if (!m_db.open()) {
        qDebug() << "Error opening database: " << m_db.lastError().text();
        return false;
    }
    return true;
}

void DBManager::closeDB()
{
    m_db.close();
}

bool DBManager::createTables()
{
    QSqlQuery query;
    
    // 创建用户表
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "username TEXT UNIQUE NOT NULL, "
                  "password TEXT NOT NULL)")) {
        qDebug() << "Error creating users table: " << query.lastError().text();
        return false;
    }
    
    // 创建车牌信息表
    if (!query.exec("CREATE TABLE IF NOT EXISTS plate_info ("
                  "plate_number TEXT PRIMARY KEY, "
                  "owner_name TEXT, "
                  "owner_phone TEXT, "
                  "register_time DATETIME DEFAULT CURRENT_TIMESTAMP)")) {
        qDebug() << "Error creating plate_info table: " << query.lastError().text();
        return false;
    }
    
    // 创建停车位表
    if (!query.exec("CREATE TABLE IF NOT EXISTS parking_spaces ("
                  "space_id INTEGER PRIMARY KEY, "
                  "status TEXT DEFAULT 'available')")) {
        qDebug() << "Error creating parking_spaces table: " << query.lastError().text();
        return false;
    }
    
    // 创建预约记录表
    if (!query.exec("CREATE TABLE IF NOT EXISTS reservations ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "space_id INTEGER, "
                  "plate_number TEXT, "
                  "start_time DATETIME, "
                  "end_time DATETIME, "
                  "status TEXT DEFAULT 'reserved', "
                  "FOREIGN KEY(space_id) REFERENCES parking_spaces(space_id), "
                  "FOREIGN KEY(plate_number) REFERENCES plate_info(plate_number))")) {
        qDebug() << "Error creating reservations table: " << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DBManager::checkUserLogin(const QString &username, const QString &password)
{
    QSqlQuery query;
    query.prepare("SELECT id FROM users WHERE username = :username AND password = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    
    if (query.exec() && query.next()) {
        return true;
    }
    return false;
}

bool DBManager::registerUser(const QString &username, const QString &password)
{
    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password) VALUES (:username, :password)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    
    if (!query.exec()) {
        qDebug() << "Error registering user: " << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::isPlateRegistered(const QString &plateNumber)
{
    QSqlQuery query;
    query.prepare("SELECT plate_number FROM plate_info WHERE plate_number = :plate_number");
    query.bindValue(":plate_number", plateNumber);
    
    if (query.exec() && query.next()) {
        return true;
    }
    return false;
}

bool DBManager::registerPlate(const QString &plateNumber, const QString &ownerName, const QString &ownerPhone)
{
    QSqlQuery query;
    query.prepare("INSERT INTO plate_info (plate_number, owner_name, owner_phone) VALUES (:plate_number, :owner_name, :owner_phone)");
    query.bindValue(":plate_number", plateNumber);
    query.bindValue(":owner_name", ownerName);
    query.bindValue(":owner_phone", ownerPhone);
    
    if (!query.exec()) {
        qDebug() << "Error registering plate: " << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::getPlateInfo(const QString &plateNumber, QString &ownerName, QString &ownerPhone)
{
    QSqlQuery query;
    query.prepare("SELECT owner_name, owner_phone FROM plate_info WHERE plate_number = :plate_number");
    query.bindValue(":plate_number", plateNumber);
    
    if (query.exec() && query.next()) {
        ownerName = query.value(0).toString();
        ownerPhone = query.value(1).toString();
        return true;
    }
    return false;
}

bool DBManager::initParkingLot(int totalSpaces)
{
    // 清空停车场表
    QSqlQuery query;
    if (!query.exec("DELETE FROM parking_spaces")) {
        qDebug() << "Error clearing parking spaces: " << query.lastError().text();
        return false;
    }
    
    // 初始化停车位
    for (int i = 1; i <= totalSpaces; i++) {
        if (!query.exec(QString("INSERT INTO parking_spaces (space_id, status) VALUES (%1, 'available')").arg(i))) {
            qDebug() << "Error initializing parking space: " << query.lastError().text();
            return false;
        }
    }
    return true;
}

QList<int> DBManager::getAvailableSpaces()
{
    QList<int> availableSpaces;
    QSqlQuery query;
    
    if (query.exec("SELECT space_id FROM parking_spaces WHERE status = 'available'")) {
        while (query.next()) {
            availableSpaces.append(query.value(0).toInt());
        }
    }
    
    return availableSpaces;
}

bool DBManager::reserveSpace(int spaceId, const QString &plateNumber, const QDateTime &startTime, const QDateTime &endTime)
{
    // 检查车位是否可用
    if (!isSpaceAvailable(spaceId, startTime, endTime)) {
        return false;
    }
    
    // 预约车位
    QSqlQuery query;
    query.prepare("INSERT INTO reservations (space_id, plate_number, start_time, end_time) "
                 "VALUES (:space_id, :plate_number, :start_time, :end_time)");
    query.bindValue(":space_id", spaceId);
    query.bindValue(":plate_number", plateNumber);
    query.bindValue(":start_time", startTime.toString(Qt::ISODate));
    query.bindValue(":end_time", endTime.toString(Qt::ISODate));
    
    if (!query.exec()) {
        qDebug() << "Error reserving space: " << query.lastError().text();
        return false;
    }
    
    // 更新车位状态
    query.prepare("UPDATE parking_spaces SET status = 'reserved' WHERE space_id = :space_id");
    query.bindValue(":space_id", spaceId);
    
    if (!query.exec()) {
        qDebug() << "Error updating space status: " << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DBManager::cancelReservation(int spaceId, const QString &plateNumber)
{
    QSqlQuery query;
    query.prepare("UPDATE reservations SET status = 'cancelled' "
                 "WHERE space_id = :space_id AND plate_number = :plate_number AND status = 'reserved'");
    query.bindValue(":space_id", spaceId);
    query.bindValue(":plate_number", plateNumber);
    
    if (!query.exec()) {
        qDebug() << "Error cancelling reservation: " << query.lastError().text();
        return false;
    }
    
    // 更新车位状态
    query.prepare("UPDATE parking_spaces SET status = 'available' WHERE space_id = :space_id");
    query.bindValue(":space_id", spaceId);
    
    if (!query.exec()) {
        qDebug() << "Error updating space status: " << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DBManager::checkIn(int spaceId, const QString &plateNumber)
{
    QSqlQuery query;
    query.prepare("UPDATE reservations SET status = 'occupied' "
                 "WHERE space_id = :space_id AND plate_number = :plate_number AND status = 'reserved'");
    query.bindValue(":space_id", spaceId);
    query.bindValue(":plate_number", plateNumber);
    
    if (!query.exec()) {
        qDebug() << "Error checking in: " << query.lastError().text();
        return false;
    }
    
    // 更新车位状态
    query.prepare("UPDATE parking_spaces SET status = 'occupied' WHERE space_id = :space_id");
    query.bindValue(":space_id", spaceId);
    
    if (!query.exec()) {
        qDebug() << "Error updating space status: " << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DBManager::checkOut(int spaceId, const QString &plateNumber)
{
    QSqlQuery query;
    query.prepare("UPDATE reservations SET status = 'completed' "
                 "WHERE space_id = :space_id AND plate_number = :plate_number AND status = 'occupied'");
    query.bindValue(":space_id", spaceId);
    query.bindValue(":plate_number", plateNumber);
    
    if (!query.exec()) {
        qDebug() << "Error checking out: " << query.lastError().text();
        return false;
    }
    
    // 更新车位状态
    query.prepare("UPDATE parking_spaces SET status = 'available' WHERE space_id = :space_id");
    query.bindValue(":space_id", spaceId);
    
    if (!query.exec()) {
        qDebug() << "Error updating space status: " << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DBManager::isSpaceAvailable(int spaceId, const QDateTime &startTime, const QDateTime &endTime)
{
    QSqlQuery query;
    query.prepare("SELECT r.id FROM reservations r "
                 "WHERE r.space_id = :space_id AND r.status IN ('reserved', 'occupied') "
                 "AND NOT (r.end_time <= :start_time OR r.start_time >= :end_time)");
    query.bindValue(":space_id", spaceId);
    query.bindValue(":start_time", startTime.toString(Qt::ISODate));
    query.bindValue(":end_time", endTime.toString(Qt::ISODate));
    
    if (query.exec() && query.next()) {
        // 存在时间冲突的预约
        return false;
    }
    
    // 检查车位当前状态
    query.prepare("SELECT status FROM parking_spaces WHERE space_id = :space_id");
    query.bindValue(":space_id", spaceId);
    
    if (query.exec() && query.next()) {
        QString status = query.value(0).toString();
        if (status == "available" || status == "reserved") {
            return true;
        }
    }
    
    return false;
}

QList<QPair<int, QString>> DBManager::getAllOccupiedSpaces()
{
    QList<QPair<int, QString>> occupiedSpaces;
    QSqlQuery query;
    
    if (query.exec("SELECT ps.space_id, r.plate_number FROM parking_spaces ps "
                  "JOIN reservations r ON ps.space_id = r.space_id "
                  "WHERE ps.status = 'occupied' AND r.status = 'occupied'")) {
        while (query.next()) {
            int spaceId = query.value(0).toInt();
            QString plateNumber = query.value(1).toString();
            occupiedSpaces.append(qMakePair(spaceId, plateNumber));
        }
    }
    
    return occupiedSpaces;
} 