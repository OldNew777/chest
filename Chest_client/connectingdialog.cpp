#include "connectingdialog.h"
#include "ui_connectingdialog.h"

#include <QDebug>
#include <QMessageBox>
#include <QStringList>

connectingDialog::connectingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::connectingDialog)
{
    ui->setupUi(this);

    setFixedSize(size());
    connect(ui->pushButton, &QPushButton::clicked, [=]{
        QString ip_in = ui->lineEdit->text();
        if (illegal(ip_in))
        {
            QMessageBox::critical(this, "Error", "IP adresss format wrong! ");
            this->hide();
            connectingDialog * cd = new connectingDialog(parent);
            cd->ip = ip;
            cd->exec();
            return;
        }
        *ip = ip_in;
    });
}

connectingDialog::~connectingDialog()
{
    delete ui;
}

bool connectingDialog::illegal(QString ip_in)
{
    QStringList sl = ip_in.split('.');
    if (sl.length() != 4)
        return true;
    for (int i = 0; i < 4; i++)
    {
        if (sl[i].length() > 3)
            return true;
        for (int j = 0; j < sl[i].length(); j++)
            if (sl[i][j] < '0' || sl[i][j] > '9')
                return true;
        int k = sl[i].toInt();
        if (k > 255)
            return true;
        if (i == 0 && k == 0)
            return true;
    }
    return false;
}
