#include "loginwindow.h"
#include "ui_loginwindow.h"

LoginWindow::LoginWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoginWindow),
    m_dbManager(new DBManager(this))
{
    ui->setupUi(this);
    setWindowTitle("登录");
    
    // 连接信号和槽
    connect(ui->loginButton, &QPushButton::clicked, this, &LoginWindow::on_loginButton_clicked);
    connect(ui->registerButton, &QPushButton::clicked, this, &LoginWindow::on_registerButton_clicked);
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::on_loginButton_clicked()
{
    QString username = ui->usernameLineEdit->text();
    QString password = ui->passwordLineEdit->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "错误", "用户名和密码不能为空！");
        return;
    }
    
    if (m_dbManager->checkUserLogin(username, password)) {
        QMessageBox::information(this, "成功", "登录成功！");
        emit loginSuccess();
    } else {
        QMessageBox::warning(this, "错误", "用户名或密码错误！");
    }
}

void LoginWindow::on_registerButton_clicked()
{
    QString username = ui->usernameLineEdit->text();
    QString password = ui->passwordLineEdit->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "错误", "用户名和密码不能为空！");
        return;
    }
    
    if (m_dbManager->registerUser(username, password)) {
        QMessageBox::information(this, "成功", "注册成功！请登录");
    } else {
        QMessageBox::warning(this, "错误", "注册失败，用户名可能已存在！");
    }
} 