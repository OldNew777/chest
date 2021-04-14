#ifndef CONNECTINGDIALOG_H
#define CONNECTINGDIALOG_H

#include <QDialog>

namespace Ui {
class connectingDialog;
}

class connectingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit connectingDialog(QWidget *parent = nullptr);
    ~connectingDialog();

    QString * ip;

private:
    Ui::connectingDialog *ui;

    bool illegal(QString ip_in);
};

#endif // CONNECTINGDIALOG_H
