#ifndef PARKINGRESERVATIONWINDOW_H
#define PARKINGRESERVATIONWINDOW_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QDateTime>
#include <QMessageBox>
#include <QList>
#include <QMap>
#include "dbmanager.h"

namespace Ui {
class ParkingReservationWindow;
}

class ParkingReservationWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ParkingReservationWindow(QWidget *parent = nullptr);
    ~ParkingReservationWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void onPlateRecognized(const QString &plateNumber);

private slots:
    void on_initParkingLotButton_clicked();
    void on_reserveButton_clicked();
    void on_cancelReservationButton_clicked();
    void on_refreshButton_clicked();
    void updateParkingLotView();

private:
    Ui::ParkingReservationWindow *ui;
    DBManager *m_dbManager;
    QGraphicsScene *m_scene;
    QMap<int, QGraphicsRectItem*> m_parkingSpaces;
    QString m_currentPlateNumber;
    int m_selectedSpaceId;
    
    void drawParkingLot(int rows, int cols);
    void colorSpaceByStatus(int spaceId, const QString &status);
    void handleGraphicsViewClick(QPointF position);
};

#endif // PARKINGRESERVATIONWINDOW_H 