#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <QtEndian>
#include <QMouseEvent>
#include <QMap>

#include "listeningdialog.h"
#include "order.h"


class Chest;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private:
    //ui和系统绘图设置
    Ui::MainWindow *ui;
    int x_start = 80, y_start = 80, width = 600;
    float grid;
    bool _start = false, _ready = false;

    int timerid = -100;

    //------------------------------------
    //棋盘设置
    Chest * chest = nullptr;
    QString pictureName[3][7], pieceName[7];
    QPixmap picture[3][7];

    bool haveAskedForDraw = false;

    Chest * chest_load = nullptr;
    QMap<QString, int> pieceNumber;


    //捏着棋子
    bool hold = false;
    Position hold_position;
    bool available[9][9];

    //兵的升变
    Kind promotionKind = Queen;

    //------------------------------------

    //网络设置
    QTcpServer * listenSocket = nullptr;
    QTcpSocket * readWriteSocket = nullptr;

    ListeningDialog * lisDia = nullptr;


    //-----------------------------------
    friend class Chest;
    friend class ListeningDialog;

    //-----------------------------------
    void doOrder(Order & order_now);

    void win_black();
    void win_white();
    void Adraw();
    void timerOut();

    //
    bool can_go(Piece piece_now[9][9], Position from, Position target);
    void calculateAvailable(Piece piece_now[9][9]);

    void kill_timer(int & timerid);

    void paintEvent(QPaintEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void timerEvent(QTimerEvent * event);

public slots:
    void initServer();
    void connectHost();
    void acceptConnection();
    void cancelListening();

    void receiveMessage();
    void sendMessage(const Order & order_now);


    //-------------------------
    void setPromotion(int k);
    void loadFile();
    void saveFile();
};

#endif // MAINWINDOW_H
