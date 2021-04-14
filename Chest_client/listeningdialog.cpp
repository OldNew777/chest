#include "listeningdialog.h"
#include "ui_listeningdialog.h"

#include "mainwindow.h"

ListeningDialog::ListeningDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ListeningDialog)
{
    ui->setupUi(this);

    setFixedSize(size());

    connect(ui->pushButton_cancel, SIGNAL(clicked()), parent, SLOT(cancelListening()));
    connect(ui->pushButton_start, &QPushButton::clicked, [=]{
        father->listenSocket->listen(QHostAddress::AnyIPv4,7633);
        connect(father->listenSocket,SIGNAL(newConnection()),father,SLOT(acceptConnection()));
        ui->pushButton_start->setEnabled(false);
        ui->pushButton_cancel->setEnabled(true);
        ui->label->setText(QString("正在监听...") % ui->label->text());
    });
}

ListeningDialog::~ListeningDialog()
{
    delete ui;
}

void ListeningDialog::setIP(const QHostAddress &address)
{
    QString add = address.toString();
    ui->lineEdit->setText(add);
}
