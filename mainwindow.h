#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "loginwindow.h"
#include "platerecognitionwindow.h"
#include "parkingreservationwindow.h"
#include "dbmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoginSuccess();
    void on_actionPlateRecognition_triggered();
    void on_actionParkingReservation_triggered();
    void on_actionLogout_triggered();

private:
    Ui::MainWindow *ui;
    QStackedWidget *m_stackedWidget;
    LoginWindow *m_loginWindow;
    PlateRecognitionWindow *m_plateRecognitionWindow;
    ParkingReservationWindow *m_parkingReservationWindow;
    DBManager *m_dbManager;
};
#endif // MAINWINDOW_H
