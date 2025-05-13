#include "platerecognitionwindow.h"
#include "ui_platerecognitionwindow.h"
#include <QDebug>
#include <QInputDialog>

PlateRecognitionWindow::PlateRecognitionWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlateRecognitionWindow),
    m_dbManager(new DBManager(this)),
    m_timer(new QTimer(this))
{
    ui->setupUi(this);
    setWindowTitle("车牌识别");
    
    // 连接信号和槽
    connect(m_timer, &QTimer::timeout, this, &PlateRecognitionWindow::processFrame);
    
    // 初始化UI
    ui->captureButton->setEnabled(false);
    ui->registerPlateButton->setEnabled(false);
}

PlateRecognitionWindow::~PlateRecognitionWindow()
{
    stopCamera();
    delete ui;
}

void PlateRecognitionWindow::on_selectImageButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择图片", "", "图片文件 (*.jpg *.jpeg *.png *.bmp)");
    if (filePath.isEmpty()) {
        return;
    }
    
    m_currentFrame = cv::imread(filePath.toStdString());
    if (m_currentFrame.empty()) {
        QMessageBox::warning(this, "错误", "无法打开图片！");
        return;
    }
    
    showImage(m_currentFrame);
}

void PlateRecognitionWindow::on_recognizeButton_clicked()
{
    if (m_currentFrame.empty()) {
        QMessageBox::warning(this, "错误", "请先选择或捕获图片！");
        return;
    }
    
    // 识别车牌
    m_recognizedPlate = recognizePlate(m_currentFrame);
    
    if (m_recognizedPlate.isEmpty()) {
        QMessageBox::warning(this, "识别结果", "未能识别到车牌！");
        ui->registerPlateButton->setEnabled(false);
        return;
    }
    
    ui->plateNumberLineEdit->setText(m_recognizedPlate);
    ui->registerPlateButton->setEnabled(true);
    
    // 检查数据库中是否已有该车牌
    QString ownerName, ownerPhone;
    if (m_dbManager->getPlateInfo(m_recognizedPlate, ownerName, ownerPhone)) {
        ui->ownerNameLineEdit->setText(ownerName);
        ui->ownerPhoneLineEdit->setText(ownerPhone);
        QMessageBox::information(this, "识别结果", "车牌已登记：" + m_recognizedPlate);
        emit plateRecognized(m_recognizedPlate);
    } else {
        QMessageBox::information(this, "识别结果", "识别到未登记车牌：" + m_recognizedPlate + "，请完善车主信息并注册");
    }
}

void PlateRecognitionWindow::on_openCameraButton_clicked()
{
    if (m_timer->isActive()) {
        stopCamera();
        ui->openCameraButton->setText("打开相机");
        ui->captureButton->setEnabled(false);
    } else {
        // 打开默认相机
        m_capture.open(0);
        if (!m_capture.isOpened()) {
            QMessageBox::warning(this, "错误", "无法打开相机！");
            return;
        }
        
        m_timer->start(30); // 30ms刷新，约33帧/秒
        ui->openCameraButton->setText("关闭相机");
        ui->captureButton->setEnabled(true);
    }
}

void PlateRecognitionWindow::on_captureButton_clicked()
{
    if (m_currentFrame.empty()) {
        QMessageBox::warning(this, "错误", "没有可用的图像！");
        return;
    }
    
    // 捕获当前帧
    showImage(m_currentFrame);
    
    // 暂停相机
    if (m_timer->isActive()) {
        m_timer->stop();
        ui->openCameraButton->setText("打开相机");
        ui->captureButton->setEnabled(false);
    }
}

void PlateRecognitionWindow::on_registerPlateButton_clicked()
{
    if (m_recognizedPlate.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先识别车牌！");
        return;
    }
    
    QString ownerName = ui->ownerNameLineEdit->text();
    QString ownerPhone = ui->ownerPhoneLineEdit->text();
    
    if (ownerName.isEmpty() || ownerPhone.isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入车主姓名和电话！");
        return;
    }
    
    if (m_dbManager->registerPlate(m_recognizedPlate, ownerName, ownerPhone)) {
        QMessageBox::information(this, "成功", "车牌注册成功！");
        emit plateRecognized(m_recognizedPlate);
    } else {
        QMessageBox::warning(this, "错误", "车牌注册失败！");
    }
}

void PlateRecognitionWindow::processFrame()
{
    if (!m_capture.isOpened()) {
        return;
    }
    
    m_capture >> m_currentFrame;
    if (m_currentFrame.empty()) {
        return;
    }
    
    // 显示当前帧
    cv::Mat displayFrame;
    cv::cvtColor(m_currentFrame, displayFrame, cv::COLOR_BGR2RGB);
    
    QImage image((uchar*)displayFrame.data, displayFrame.cols, displayFrame.rows, 
                displayFrame.step, QImage::Format_RGB888);
    ui->imageLabel->setPixmap(QPixmap::fromImage(image).scaled(ui->imageLabel->size(), 
                                                             Qt::KeepAspectRatio, 
                                                             Qt::SmoothTransformation));
}

QString PlateRecognitionWindow::recognizePlate(const cv::Mat &image)
{
    // 简化版车牌识别
    // 实际项目中应该使用OpenCV以及可能的第三方库来识别车牌
    // 这里仅作为示例，返回一个固定的车牌号码或从用户输入获取
    
    // 这里可以实现车牌识别算法
    // 但由于复杂度较高，这里仅仅模拟识别过程
    
    // 模拟车牌识别（实际项目应替换为真实的车牌识别算法）
    // 简单起见，让用户输入识别结果，以模拟实际识别
    bool ok;
    QString plateNumber = QInputDialog::getText(this, "车牌识别结果", 
                                              "请输入识别的车牌号码(实际项目中应为自动识别):", 
                                              QLineEdit::Normal, 
                                              "粤B12345", &ok);
    if (ok && !plateNumber.isEmpty()) {
        return plateNumber;
    }
    
    return QString();
}

void PlateRecognitionWindow::showImage(const cv::Mat &image)
{
    if (image.empty()) {
        return;
    }
    
    cv::Mat displayImage;
    cv::cvtColor(image, displayImage, cv::COLOR_BGR2RGB);
    
    QImage qImage((uchar*)displayImage.data, displayImage.cols, displayImage.rows, 
                 displayImage.step, QImage::Format_RGB888);
    ui->imageLabel->setPixmap(QPixmap::fromImage(qImage).scaled(ui->imageLabel->size(), 
                                                               Qt::KeepAspectRatio, 
                                                               Qt::SmoothTransformation));
}

void PlateRecognitionWindow::stopCamera()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
    
    if (m_capture.isOpened()) {
        m_capture.release();
    }
} 