#include "parkingreservationwindow.h"
#include "ui_parkingreservationwindow.h"
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QSqlQuery>
#include <QEvent>
#include <QMouseEvent>

ParkingReservationWindow::ParkingReservationWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParkingReservationWindow),
    m_dbManager(new DBManager(this)),
    m_selectedSpaceId(-1)
{
    ui->setupUi(this);
    setWindowTitle("停车场预约");
    
    // 创建场景
    m_scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(m_scene);
    
    // 安装事件过滤器来捕获鼠标点击事件
    ui->graphicsView->viewport()->installEventFilter(this);
    
    // 设置日期时间编辑器的范围
    QDateTime currentTime = QDateTime::currentDateTime();
    ui->startDateTimeEdit->setDateTime(currentTime.addSecs(60));
    ui->startDateTimeEdit->setMinimumDateTime(currentTime);
    ui->endDateTimeEdit->setDateTime(currentTime.addSecs(3600));
    ui->endDateTimeEdit->setMinimumDateTime(currentTime.addSecs(600)); // 至少10分钟
    
    // 禁用预约按钮，直到选择车位和输入车牌
    ui->reserveButton->setEnabled(false);
    ui->cancelReservationButton->setEnabled(false);
    
    // 默认初始化一个4x5的停车场
    drawParkingLot(4, 5);
    m_dbManager->initParkingLot(20);
    updateParkingLotView();
}

ParkingReservationWindow::~ParkingReservationWindow()
{
    delete ui;
}

bool ParkingReservationWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->graphicsView->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QPointF scenePos = ui->graphicsView->mapToScene(mouseEvent->pos());
            handleGraphicsViewClick(scenePos);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ParkingReservationWindow::onPlateRecognized(const QString &plateNumber)
{
    m_currentPlateNumber = plateNumber;
    ui->plateNumberLineEdit->setText(plateNumber);
    
    // 更新预约按钮状态
    ui->reserveButton->setEnabled(m_selectedSpaceId != -1 && !plateNumber.isEmpty());
}

void ParkingReservationWindow::handleGraphicsViewClick(QPointF position)
{
    // 查找被点击的车位
    for (auto it = m_parkingSpaces.begin(); it != m_parkingSpaces.end(); ++it) {
        if (it.value()->contains(it.value()->mapFromScene(position))) {
            m_selectedSpaceId = it.key();
            ui->spaceIdLabel->setText(QString::number(m_selectedSpaceId));
            
            // 检查车位是否可用
            QDateTime startTime = ui->startDateTimeEdit->dateTime();
            QDateTime endTime = ui->endDateTimeEdit->dateTime();
            
            if (m_dbManager->isSpaceAvailable(m_selectedSpaceId, startTime, endTime)) {
                ui->statusLabel->setText("可用");
                ui->reserveButton->setEnabled(!m_currentPlateNumber.isEmpty());
                ui->cancelReservationButton->setEnabled(false);
            } else {
                ui->statusLabel->setText("已预约或占用");
                ui->reserveButton->setEnabled(false);
                
                // 检查是否是当前车牌的预约
                QSqlQuery query;
                query.prepare("SELECT id FROM reservations WHERE space_id = :space_id AND plate_number = :plate_number AND status = 'reserved'");
                query.bindValue(":space_id", m_selectedSpaceId);
                query.bindValue(":plate_number", m_currentPlateNumber);
                
                if (query.exec() && query.next()) {
                    ui->cancelReservationButton->setEnabled(true);
                } else {
                    ui->cancelReservationButton->setEnabled(false);
                }
            }
            
            return;
        }
    }
    
    // 如果点击在空白区域
    m_selectedSpaceId = -1;
    ui->spaceIdLabel->setText("无");
    ui->statusLabel->setText("未选择");
    ui->reserveButton->setEnabled(false);
    ui->cancelReservationButton->setEnabled(false);
}

void ParkingReservationWindow::on_initParkingLotButton_clicked()
{
    int rows = ui->rowsSpinBox->value();
    int cols = ui->colsSpinBox->value();
    int totalSpaces = rows * cols;
    
    if (QMessageBox::question(this, "确认初始化", 
                           QString("确定要初始化停车场为 %1 x %2 = %3 个车位吗？所有现有数据将被清除！")
                           .arg(rows).arg(cols).arg(totalSpaces)) == QMessageBox::Yes) {
        drawParkingLot(rows, cols);
        m_dbManager->initParkingLot(totalSpaces);
        updateParkingLotView();
        
        QMessageBox::information(this, "成功", "停车场初始化完成！");
    }
}

void ParkingReservationWindow::on_reserveButton_clicked()
{
    if (m_selectedSpaceId == -1) {
        QMessageBox::warning(this, "错误", "请先选择一个车位！");
        return;
    }
    
    if (m_currentPlateNumber.isEmpty()) {
        m_currentPlateNumber = ui->plateNumberLineEdit->text();
        if (m_currentPlateNumber.isEmpty()) {
            QMessageBox::warning(this, "错误", "请输入车牌号码！");
            return;
        }
    }
    
    QDateTime startTime = ui->startDateTimeEdit->dateTime();
    QDateTime endTime = ui->endDateTimeEdit->dateTime();
    
    if (startTime >= endTime) {
        QMessageBox::warning(this, "错误", "结束时间必须晚于开始时间！");
        return;
    }
    
    if (m_dbManager->reserveSpace(m_selectedSpaceId, m_currentPlateNumber, startTime, endTime)) {
        QMessageBox::information(this, "成功", QString("成功预约车位 %1").arg(m_selectedSpaceId));
        updateParkingLotView();
    } else {
        QMessageBox::warning(this, "错误", "预约失败，车位可能已被占用！");
    }
}

void ParkingReservationWindow::on_cancelReservationButton_clicked()
{
    if (m_selectedSpaceId == -1) {
        QMessageBox::warning(this, "错误", "请先选择一个车位！");
        return;
    }
    
    if (m_currentPlateNumber.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先识别车牌或输入车牌号码！");
        return;
    }
    
    if (m_dbManager->cancelReservation(m_selectedSpaceId, m_currentPlateNumber)) {
        QMessageBox::information(this, "成功", QString("成功取消车位 %1 的预约").arg(m_selectedSpaceId));
        updateParkingLotView();
    } else {
        QMessageBox::warning(this, "错误", "取消预约失败！");
    }
}

void ParkingReservationWindow::on_refreshButton_clicked()
{
    updateParkingLotView();
}

void ParkingReservationWindow::updateParkingLotView()
{
    // 获取所有车位状态
    QSqlQuery query;
    if (query.exec("SELECT space_id, status FROM parking_spaces")) {
        while (query.next()) {
            int spaceId = query.value(0).toInt();
            QString status = query.value(1).toString();
            
            if (m_parkingSpaces.contains(spaceId)) {
                colorSpaceByStatus(spaceId, status);
            }
        }
    }
    
    // 获取所有已占用车位的车牌
    QList<QPair<int, QString>> occupiedSpaces = m_dbManager->getAllOccupiedSpaces();
    
    // 更新车牌显示
    for (int i = 0; i < occupiedSpaces.size(); ++i) {
        int spaceId = occupiedSpaces[i].first;
        QString plateNumber = occupiedSpaces[i].second;
        
        if (m_parkingSpaces.contains(spaceId)) {
            // 在车位上显示车牌号码
            QGraphicsTextItem *textItem = m_scene->addText(plateNumber.right(5)); // 只显示后5位
            QRectF rect = m_parkingSpaces[spaceId]->rect();
            QPointF center = m_parkingSpaces[spaceId]->mapToScene(rect.center());
            
            textItem->setPos(center.x() - textItem->boundingRect().width() / 2,
                           center.y() - textItem->boundingRect().height() / 2);
        }
    }
}

void ParkingReservationWindow::drawParkingLot(int rows, int cols)
{
    // 清除现有场景
    m_scene->clear();
    m_parkingSpaces.clear();
    
    // 停车位的大小
    int spaceWidth = 60;
    int spaceHeight = 100;
    int spacing = 10;
    
    // 计算所需的场景大小
    int sceneWidth = cols * (spaceWidth + spacing) + spacing;
    int sceneHeight = rows * (spaceHeight + spacing) + spacing;
    
    m_scene->setSceneRect(0, 0, sceneWidth, sceneHeight);
    
    // 绘制停车位
    int spaceId = 1;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            int x = spacing + col * (spaceWidth + spacing);
            int y = spacing + row * (spaceHeight + spacing);
            
            QGraphicsRectItem *space = m_scene->addRect(x, y, spaceWidth, spaceHeight);
            space->setPen(QPen(Qt::black, 2));
            space->setBrush(QBrush(Qt::green)); // 默认为可用状态
            m_parkingSpaces[spaceId] = space;
            
            // 添加车位编号
            QGraphicsTextItem *textItem = m_scene->addText(QString::number(spaceId));
            textItem->setPos(x + spaceWidth / 2 - textItem->boundingRect().width() / 2,
                           y + 5);
            
            spaceId++;
        }
    }
    
    // 调整视图
    ui->graphicsView->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void ParkingReservationWindow::colorSpaceByStatus(int spaceId, const QString &status)
{
    if (!m_parkingSpaces.contains(spaceId)) {
        return;
    }
    
    if (status == "available") {
        m_parkingSpaces[spaceId]->setBrush(QBrush(Qt::green));
    } else if (status == "reserved") {
        m_parkingSpaces[spaceId]->setBrush(QBrush(Qt::yellow));
    } else if (status == "occupied") {
        m_parkingSpaces[spaceId]->setBrush(QBrush(Qt::red));
    }
} 