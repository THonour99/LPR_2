#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_dbManager(new DBManager(this))
{
    ui->setupUi(this);
    setWindowTitle("停车场预约管理系统");
    
    // 创建堆栈窗口部件并设置为中心窗口部件
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);
    
    // 创建子窗口
    m_loginWindow = new LoginWindow();
    m_plateRecognitionWindow = new PlateRecognitionWindow();
    m_parkingReservationWindow = new ParkingReservationWindow();
    
    // 将子窗口添加到堆栈窗口部件中
    m_stackedWidget->addWidget(m_loginWindow);
    m_stackedWidget->addWidget(m_plateRecognitionWindow);
    m_stackedWidget->addWidget(m_parkingReservationWindow);
    
    // 连接子窗口的信号和槽
    connect(m_loginWindow, &LoginWindow::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(m_plateRecognitionWindow, &PlateRecognitionWindow::plateRecognized, 
            m_parkingReservationWindow, &ParkingReservationWindow::onPlateRecognized);
    
    // 初始显示登录窗口
    m_stackedWidget->setCurrentWidget(m_loginWindow);
    
    // 禁用菜单操作，直到登录成功
    ui->menuParkingSystem->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onLoginSuccess()
{
    // 登录成功后，启用菜单操作
    ui->menuParkingSystem->setEnabled(true);
    
    // 切换到车牌识别窗口
    m_stackedWidget->setCurrentWidget(m_plateRecognitionWindow);
}

void MainWindow::on_actionPlateRecognition_triggered()
{
    m_stackedWidget->setCurrentWidget(m_plateRecognitionWindow);
}

void MainWindow::on_actionParkingReservation_triggered()
{
    m_stackedWidget->setCurrentWidget(m_parkingReservationWindow);
}

void MainWindow::on_actionLogout_triggered()
{
    // 禁用菜单操作
    ui->menuParkingSystem->setEnabled(false);
    
    // 切换回登录窗口
    m_stackedWidget->setCurrentWidget(m_loginWindow);
}
