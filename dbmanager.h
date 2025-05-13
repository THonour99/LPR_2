#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>

class DBManager : public QObject
{
    Q_OBJECT
public:
    explicit DBManager(QObject *parent = nullptr);
    ~DBManager();

    bool openDB();
    void closeDB();
    bool createTables();
    
    // 用户管理
    bool checkUserLogin(const QString &username, const QString &password);
    bool registerUser(const QString &username, const QString &password);
    
    // 车牌管理
    bool isPlateRegistered(const QString &plateNumber);
    bool registerPlate(const QString &plateNumber, const QString &ownerName, const QString &ownerPhone);
    bool getPlateInfo(const QString &plateNumber, QString &ownerName, QString &ownerPhone);
    
    // 停车场管理
    bool initParkingLot(int totalSpaces);
    QList<int> getAvailableSpaces();
    bool reserveSpace(int spaceId, const QString &plateNumber, const QDateTime &startTime, const QDateTime &endTime);
    bool cancelReservation(int spaceId, const QString &plateNumber);
    bool checkIn(int spaceId, const QString &plateNumber);
    bool checkOut(int spaceId, const QString &plateNumber);
    bool isSpaceAvailable(int spaceId, const QDateTime &startTime, const QDateTime &endTime);
    QList<QPair<int, QString>> getAllOccupiedSpaces();

private:
    QSqlDatabase m_db;
};

#endif // DBMANAGER_H 