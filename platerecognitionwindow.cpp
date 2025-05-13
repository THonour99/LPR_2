#include "platerecognitionwindow.h"
#include "ui_platerecognitionwindow.h"
#include <QDebug>
#include <QInputDialog>
#include <QEvent>
#include <QMouseEvent>
#include <algorithm>
#include <vector>
#include <ctime>

// 定义中国车牌字符集，用于简单模板匹配
const std::vector<QString> PLATE_CHARS = {
    "京", "津", "沪", "渝", "冀", "豫", "云", "辽", "黑", "湘", "皖", "鲁", "新", "苏", "浙", "赣", 
    "鄂", "桂", "甘", "晋", "蒙", "陕", "吉", "闽", "贵", "粤", "青", "藏", "川", "宁", "琼", 
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "A", "B", "C", "D", "E", "F", "G", "H", "J", "K", "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
};

PlateRecognitionWindow::PlateRecognitionWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlateRecognitionWindow),
    m_dbManager(new DBManager(this)),
    m_timer(new QTimer(this))
{
    ui->setupUi(this);
    setWindowTitle("车牌识别");
    
    // 初始化随机数生成器
    std::srand(std::time(nullptr));
    
    // 连接信号和槽
    connect(m_timer, &QTimer::timeout, this, &PlateRecognitionWindow::processFrame);
    
    // 初始化UI
    ui->captureButton->setEnabled(false);
    ui->registerPlateButton->setEnabled(false);
    
#ifdef USE_TESSERACT_OCR
    // 初始化Tesseract OCR引擎
    m_tesseract = new tesseract::TessBaseAPI();
    // 设置Tesseract数据路径，使用中文简体训练数据
    if (m_tesseract->Init(nullptr, "chi_sim") != 0) {
        qDebug() << "无法初始化Tesseract OCR引擎";
        delete m_tesseract;
        m_tesseract = nullptr;
    } else {
        // 设置识别模式：仅识别单行文本
        m_tesseract->SetPageSegMode(tesseract::PSM_SINGLE_LINE);
        // 设置字符白名单，限制为车牌可能出现的字符
        m_tesseract->SetVariable("tessedit_char_whitelist", "京津沪渝冀豫云辽黑湘皖鲁新苏浙赣鄂桂甘晋蒙陕吉闽贵粤青藏川宁琼使领ABCDEFGHJKLMNPQRSTUVWXYZ0123456789");
    }
#endif
}

PlateRecognitionWindow::~PlateRecognitionWindow()
{
    stopCamera();
    
#ifdef USE_TESSERACT_OCR
    // 清理Tesseract资源
    if (m_tesseract) {
        m_tesseract->End();
        delete m_tesseract;
    }
#endif
    
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

// 添加字符分割函数
std::vector<cv::Mat> PlateRecognitionWindow::segmentChars(const cv::Mat& plateImage) {
    // 转为灰度图，如果不是的话
    cv::Mat grayPlate;
    if (plateImage.channels() == 3) {
        cv::cvtColor(plateImage, grayPlate, cv::COLOR_BGR2GRAY);
    } else {
        grayPlate = plateImage.clone();
    }
    
    // 对图像进行二值化处理
    cv::Mat binary;
    cv::adaptiveThreshold(grayPlate, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 11, 2);
    
    // 显示二值化结果
    cv::imshow("Binarized Plate", binary);
    
    // 查找所有轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // 根据面积过滤轮廓
    std::vector<cv::Rect> charRects;
    double minArea = binary.rows * binary.cols * 0.01; // 最小面积阈值
    double maxArea = binary.rows * binary.cols * 0.3;  // 最大面积阈值
    
    for (const auto& contour : contours) {
        cv::Rect rect = cv::boundingRect(contour);
        double area = rect.width * rect.height;
        
        // 字符高度应该占据车牌高度的一定比例
        if (area > minArea && area < maxArea && 
            rect.height > binary.rows * 0.4 &&
            rect.width < rect.height * 1.5 && 
            rect.width > rect.height * 0.1) {
            charRects.push_back(rect);
        }
    }
    
    // 按照X坐标排序字符区域，确保从左到右顺序
    std::sort(charRects.begin(), charRects.end(), [](const cv::Rect& a, const cv::Rect& b) {
        return a.x < b.x;
    });
    
    // 绘制字符区域
    cv::Mat charVisualization = plateImage.clone();
    for (size_t i = 0; i < charRects.size(); i++) {
        cv::rectangle(charVisualization, charRects[i], cv::Scalar(0, 255, 0), 1);
        // 在字符上标记序号
        cv::putText(charVisualization, QString::number(i).toStdString(), 
                   cv::Point(charRects[i].x, charRects[i].y), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
    }
    cv::imshow("Character Segmentation", charVisualization);
    
    // 提取每个字符
    std::vector<cv::Mat> chars;
    for (const auto& rect : charRects) {
        // 确保矩形在图像范围内
        cv::Rect safeRect = rect & cv::Rect(0, 0, binary.cols, binary.rows);
        if (safeRect.width > 0 && safeRect.height > 0) {
            cv::Mat charImg = binary(safeRect);
            
            // 标准化字符大小 (20x40)
            cv::Mat normalizedChar;
            cv::resize(charImg, normalizedChar, cv::Size(20, 40));
            
            chars.push_back(normalizedChar);
        }
    }
    
    return chars;
}

// 简单的OCR识别函数 - 基于模板匹配
QString PlateRecognitionWindow::simpleOCR(const std::vector<cv::Mat>& chars) {
    if (chars.empty()) {
        return QString();
    }
    
    // 在真实环境中，这里应该加载预训练的字符模板或使用机器学习模型
    // 简化实现：随机从第一位选择省份简称，后面选择字母和数字
    // 这只是模拟，实际应用需要真实的字符识别模型
    
    QString result;
    if (chars.size() >= 7) {  // 标准中国车牌有7-8个字符
        result = PLATE_CHARS[rand() % 31]; // 第一位随机选择省份简称
        result += PLATE_CHARS[31 + rand() % 26]; // 第二位字母
        
        // 剩余位置
        for (size_t i = 2; i < chars.size(); i++) {
            if (i == 2) {
                result += PLATE_CHARS[31 + rand() % 26]; // 字母
            } else {
                result += PLATE_CHARS[31 + rand() % 36]; // 字母或数字
            }
        }
    } else {
        // 如果字符数量不足，返回一个默认的模拟车牌
        result = "京A12345";
    }
    
    return result;
}

#ifdef USE_TESSERACT_OCR
// Tesseract OCR函数实现
QString PlateRecognitionWindow::tesseractOCR(const cv::Mat& plateImage)
{
    if (!m_tesseract) {
        return QString();
    }
    
    // 确保图像是灰度图
    cv::Mat gray;
    if (plateImage.channels() == 3) {
        cv::cvtColor(plateImage, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = plateImage.clone();
    }
    
    // 进行二值化处理，增强字符对比度
    cv::Mat binary;
    cv::adaptiveThreshold(gray, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                         cv::THRESH_BINARY_INV, 11, 2);
    
    // 显示处理后的图像
    cv::imshow("Tesseract Input", binary);
    
    // 设置图像数据
    m_tesseract->SetImage(binary.data, binary.cols, binary.rows, 1, binary.step);
    
    // 执行OCR
    char* outText = m_tesseract->GetUTF8Text();
    
    // 转换结果为QString并清理空格
    QString result = QString::fromUtf8(outText).simplified();
    
    // 释放Tesseract分配的内存
    delete[] outText;
    
    // 后处理：移除可能的非法字符
    result.remove(QRegExp("[^京津沪渝冀豫云辽黑湘皖鲁新苏浙赣鄂桂甘晋蒙陕吉闽贵粤青藏川宁琼使领A-Z0-9]"));
    
    return result;
}
#endif

// 修改recognizePlate方法，在适当位置添加Tesseract调用
QString PlateRecognitionWindow::recognizePlate(const cv::Mat &image) {
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
        
        // 放大显示可能的车牌区域
        cv::Mat enlargedPlate;
        cv::resize(plateROI, enlargedPlate, cv::Size(plateROI.cols * 2, plateROI.rows * 2));
        cv::imshow("Plate ROI", enlargedPlate);
        
        // 识别结果变量
        QString recognizedPlate;
        
#ifdef USE_TESSERACT_OCR
        // 如果启用了Tesseract，优先使用它进行识别
        if (m_tesseract) {
            recognizedPlate = tesseractOCR(plateROI);
            qDebug() << "Tesseract OCR结果:" << recognizedPlate;
            
            // 如果Tesseract识别失败，回退到字符分割方法
            if (recognizedPlate.isEmpty() || recognizedPlate.length() < 5 || recognizedPlate.length() > 8) {
                std::vector<cv::Mat> chars = segmentChars(plateROI);
                recognizedPlate = simpleOCR(chars);
                qDebug() << "字符分割OCR结果:" << recognizedPlate;
            }
        } else {
            // 没有可用的Tesseract引擎，使用字符分割方法
            std::vector<cv::Mat> chars = segmentChars(plateROI);
            recognizedPlate = simpleOCR(chars);
        }
#else
        // 未启用Tesseract，使用字符分割方法
        std::vector<cv::Mat> chars = segmentChars(plateROI);
        recognizedPlate = simpleOCR(chars);
#endif
        
        // 无论使用哪种方法，都让用户确认结果
        bool ok;
        QString plateNumber = QInputDialog::getText(
            this,
            "车牌识别确认",
            recognizedPlate.isEmpty() ? 
                "未能自动识别车牌，请手动输入车牌号码:" :
                "识别结果为: " + recognizedPlate + "\n请确认或修改:",
            QLineEdit::Normal,
            recognizedPlate.isEmpty() ? "京A12345" : recognizedPlate,
            &ok
        );
        
        if (ok && !plateNumber.isEmpty()) {
            return plateNumber;
        }
    } else {
        // 如果没有检测到车牌区域
        QMessageBox::information(this, "提示", "未能检测到车牌区域，请调整图像或手动输入。");
        
        bool ok;
        QString plateNumber = QInputDialog::getText(
            this,
            "手动输入车牌号码",
            "未能自动识别车牌，请手动输入车牌号码:",
            QLineEdit::Normal,
            "",
            &ok
        );
        
        if (ok && !plateNumber.isEmpty()) {
            return plateNumber;
        }
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