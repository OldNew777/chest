#ifndef LISTENINGDIALOG_H
#define LISTENINGDIALOG_H

#include <QDialog>
#include <QtNetwork/QHostAddress>
#include <winsock2.h>

class MainWindow;


namespace Ui {
class ListeningDialog;
}

class ListeningDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ListeningDialog(QWidget *parent = nullptr);
    ~ListeningDialog();

    MainWindow * father;

    void setIP(const QHostAddress & address);

private:
    Ui::ListeningDialog *ui;

    friend class MainWindow;
};

#endif // LISTENINGDIALOG_H
