#ifndef PLATERECOGNITIONWINDOW_H
#define PLATERECOGNITIONWINDOW_H

#include <QWidget>
#include <QTimer>
#include <QImage>
#include <QMessageBox>
#include <QFileDialog>
#include <QPixmap>
#include <opencv2/opencv.hpp>
#include "dbmanager.h"

namespace Ui {
class PlateRecognitionWindow;
}

class PlateRecognitionWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PlateRecognitionWindow(QWidget *parent = nullptr);
    ~PlateRecognitionWindow();

signals:
    void plateRecognized(const QString &plateNumber);

private slots:
    void on_selectImageButton_clicked();
    void on_recognizeButton_clicked();
    void on_openCameraButton_clicked();
    void on_captureButton_clicked();
    void on_registerPlateButton_clicked();
    void processFrame();

private:
    Ui::PlateRecognitionWindow *ui;
    DBManager *m_dbManager;
    QTimer *m_timer;
    cv::VideoCapture m_capture;
    cv::Mat m_currentFrame;
    QString m_recognizedPlate;
    
    QString recognizePlate(const cv::Mat &image);
    void showImage(const cv::Mat &image);
    void stopCamera();
};

#endif // PLATERECOGNITIONWINDOW_H 