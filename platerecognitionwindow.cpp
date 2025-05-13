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
    // 创建原始图像的副本
    cv::Mat processedImage = image.clone();
    
    // 创建用于显示处理结果的图像
    cv::Mat debugImage = image.clone();
    
    // 1. 调整图像大小，确保宽度不超过1000像素，以提高处理速度
    double scale = 1.0;
    if (processedImage.cols > 1000) {
        scale = 1000.0 / processedImage.cols;
        cv::resize(processedImage, processedImage, cv::Size(), scale, scale);
        cv::resize(debugImage, debugImage, cv::Size(), scale, scale);
    }
    
    // 2. 转换到HSV色彩空间，车牌蓝色区域更容易提取
    cv::Mat hsv;
    cv::cvtColor(processedImage, hsv, cv::COLOR_BGR2HSV);
    
    // 3. 根据中国蓝牌的颜色范围提取蓝色区域
    cv::Mat blueMask;
    cv::inRange(hsv, cv::Scalar(100, 70, 70), cv::Scalar(140, 255, 255), blueMask);
    
    // 也可以尝试检测黄色牌照
    cv::Mat yellowMask;
    cv::inRange(hsv, cv::Scalar(15, 70, 70), cv::Scalar(40, 255, 255), yellowMask);
    
    // 合并蓝色和黄色掩码
    cv::Mat colorMask = blueMask | yellowMask;
    
    // 形态学操作，清理掩码
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::morphologyEx(colorMask, colorMask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(colorMask, colorMask, cv::MORPH_CLOSE, kernel);
    
    // 显示颜色筛选结果
    cv::imshow("Color Mask", colorMask);
    
    // 4. 转换为灰度图像
    cv::Mat gray;
    cv::cvtColor(processedImage, gray, cv::COLOR_BGR2GRAY);
    
    // 5. 高斯模糊减少噪声
    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 0);
    
    // 6. Sobel边缘检测（X方向，车牌边缘在水平方向上变化明显）
    cv::Mat sobel;
    cv::Sobel(gray, sobel, CV_8U, 1, 0, 3);
    
    // 7. 二值化
    cv::Mat binary;
    cv::threshold(sobel, binary, 0, 255, cv::THRESH_OTSU + cv::THRESH_BINARY);
    
    // 8. 形态学闭操作，填充空洞
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(17, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, element);
    
    // 显示边缘检测结果
    cv::imshow("Edge Detection", binary);
    
    // 9. 查找轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // 10. 绘制所有轮廓，用于调试
    cv::drawContours(debugImage, contours, -1, cv::Scalar(0, 255, 0), 1);
    cv::imshow("All Contours", debugImage);
    
    // 11. 遍历轮廓，寻找车牌区域
    std::vector<cv::Rect> possiblePlates;
    for (const auto& contour : contours) {
        cv::Rect rect = cv::boundingRect(contour);
        
        // 根据车牌的长宽比例和面积筛选
        double ratio = (double)rect.width / rect.height;
        double area = rect.width * rect.height;
        int minArea = 2000 * scale * scale; // 根据缩放比例调整最小面积
        
        // 中国车牌比例约为3.0-3.5，面积要足够大
        if (ratio > 2.5 && ratio < 4.5 && area > minArea) {
            // 检查这个区域是否包含足够的蓝色或黄色像素
            cv::Mat plateColorMask = colorMask(rect);
            int colorPixels = cv::countNonZero(plateColorMask);
            double colorRatio = (double)colorPixels / area;
            
            // 如果颜色匹配度较高，更可能是车牌
            if (colorRatio > 0.2) {
                possiblePlates.push_back(rect);
                
                // 在调试图像中标记潜在的车牌
                cv::rectangle(debugImage, rect, cv::Scalar(0, 0, 255), 2);
            }
        }
    }
    
    // 显示可能的车牌区域
    cv::imshow("Possible Plates", debugImage);
    
    // 12. 如果找到可能的车牌区域，进行进一步处理
    if (!possiblePlates.empty()) {
        // 按面积排序，通常最大的符合条件的矩形最可能是车牌
        std::sort(possiblePlates.begin(), possiblePlates.end(), 
                 [](const cv::Rect& a, const cv::Rect& b) {
                     return a.area() > b.area();
                 });
        
        // 取最可能的车牌区域
        cv::Rect plateRect = possiblePlates[0];
        
        // 提取车牌ROI
        cv::Mat plateROI = processedImage(plateRect);
        
        // 显示可能的车牌区域
        cv::resize(plateROI, plateROI, cv::Size(plateROI.cols * 2, plateROI.rows * 2));
        cv::imshow("Plate ROI", plateROI);
        
        // 转为灰度并进行自适应阈值处理，以准备字符分割
        cv::Mat plateGray;
        cv::cvtColor(plateROI, plateGray, cv::COLOR_BGR2GRAY);
        cv::Mat plateBinary;
        cv::adaptiveThreshold(plateGray, plateBinary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                             cv::THRESH_BINARY_INV, 11, 2);
        
        // 显示二值化的车牌
        cv::imshow("Plate Binary", plateBinary);
        
        // 这里可以添加更多的字符分割和OCR处理代码
        // 实际项目中，应该使用OCR引擎或深度学习模型进行字符识别
        
        // 由于我们没有实现完整的OCR，请用户确认识别结果
        bool ok;
        QString plateNumber = QInputDialog::getText(this, "车牌识别确认", 
                                              "检测到可能的车牌区域，请确认或修改车牌号码:",
                                              QLineEdit::Normal, 
                                              "粤B12345", &ok);
        if (ok && !plateNumber.isEmpty()) {
            return plateNumber;
        }
    }
    
    // 如果没有检测到车牌，询问用户手动输入
    bool ok;
    QString plateNumber = QInputDialog::getText(this, "手动输入车牌号码", 
                                           "未能自动识别车牌，请手动输入车牌号码:", 
                                           QLineEdit::Normal, 
                                           "", &ok);
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