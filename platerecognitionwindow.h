#ifndef PLATERECOGNITIONWINDOW_H
#define PLATERECOGNITIONWINDOW_H

// 取消下面的注释来启用Tesseract OCR
// #define USE_TESSERACT_OCR

#include <QWidget>
#include <QTimer>
#include <QImage>
#include <QMessageBox>
#include <QFileDialog>
#include <QPixmap>
#include <opencv2/opencv.hpp>
#include "dbmanager.h"
#include <vector>

#ifdef USE_TESSERACT_OCR
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#endif

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
    
    // 新增的字符分割和OCR函数
    std::vector<cv::Mat> segmentChars(const cv::Mat& plateImage);
    QString simpleOCR(const std::vector<cv::Mat>& chars);
    
#ifdef USE_TESSERACT_OCR
    // Tesseract OCR相关函数
    QString tesseractOCR(const cv::Mat& plateImage);
    tesseract::TessBaseAPI* m_tesseract;
#endif
};

#endif // PLATERECOGNITIONWINDOW_H 
