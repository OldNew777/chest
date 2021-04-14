#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "chest.h"
#include "promotiondialog.h"
#include "connectingdialog.h"

#include <QMessageBox>
#include <QDebug>
#include <QPainter>
#include <QtEndian>
#include <QFile>
#include <QFileDialog>
#include <QSound>
#include <QChar>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(size());

    //初始化数据
    grid = width / 8;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            available[i][j] = false;

    pictureName[1][1] = "white_king.png";
    pictureName[1][2] = "white_queen.png";
    pictureName[1][3] = "white_bishop.png";
    pictureName[1][4] = "white_knight.png";
    pictureName[1][5] = "white_rook.png";
    pictureName[1][6] = "white_pawn.png";

    pictureName[2][1] = "black_king.png";
    pictureName[2][2] = "black_queen.png";
    pictureName[2][3] = "black_bishop.png";
    pictureName[2][4] = "black_knight.png";
    pictureName[2][5] = "black_rook.png";
    pictureName[2][6] = "black_pawn.png";

    pieceName[1] = "king";
    pieceName[2] = "queen";
    pieceName[3] = "bishop";
    pieceName[4] = "knight";
    pieceName[5] = "rook";
    pieceName[6] = "pawn";

    pieceNumber.clear();
    pieceNumber["king"] = 1;
    pieceNumber["queen"] = 2;
    pieceNumber["bishop"] = 3;
    pieceNumber["knight"] = 4;
    pieceNumber["rook"] = 5;
    pieceNumber["pawn"] = 6;


    for (int i = 1; i <= 6; i++)
        picture[1][i].load(QString(":/picture_white/picture/white/%1").arg(pictureName[1][i]));
    for (int i = 1; i <= 6; i++)
        picture[2][i].load(QString(":/picture_black/picture/black/%1").arg(pictureName[2][i]));

    chest = new Chest;
    chest->father = this;

    connect(ui->actionListen, SIGNAL(triggered()), this, SLOT(initServer()));
    connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(connectHost()));

    //准备按钮
    connect(ui->pushButton_ready, &QPushButton::clicked, [=]{
        ui->pushButton_ready->setEnabled(false);
        if (this->_start)
            return;
        this->_ready = true;
        Order order_tmp;
        order_tmp.order_kind = Ready;
        this->sendMessage(order_tmp);
    });

    //认输按钮
    connect(ui->pushButton_giveup, &QPushButton::clicked, [=]{
        Order order_tmp;
        order_tmp.order_kind = GiveUp;
        this->doOrder(order_tmp);
    });

    //求和按钮
    connect(ui->pushButton_askfordraw, &QPushButton::clicked, [=]{
        if (this->haveAskedForDraw)
        {
            QMessageBox::information(this, "Tips", "Draw ask limit. ");
            return;
        }
        this->haveAskedForDraw = true;
        Order order_tmp;
        order_tmp.order_kind = DrawAsk;
        sendMessage(order_tmp);
    });

    //存档
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);

    //读档
    connect(ui->actionLoad, &QAction::triggered, this, &MainWindow::loadFile);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (chest)
        delete chest;

    if (listenSocket)
        delete listenSocket;

    kill_timer(timerid);
}

void MainWindow::doOrder(Order &order_now)
{
    if (order_now.order_kind == GiveUp)//认输
    {
        kill_timer(timerid);
        ui->pushButton_giveup->setEnabled(false);
        ui->pushButton_askfordraw->setEnabled(false);
        Order myorder;
        if (chest->self_party == White)
        {
            myorder.order_kind = BlackWin;
            sendMessage(myorder);
            doOrder(myorder);
        }
        else {
            myorder.order_kind = WhiteWin;
            sendMessage(myorder);
            doOrder(myorder);
        }
        return;
    }
    else if (order_now.order_kind == WhiteWin)//白胜
    {
        kill_timer(timerid);
        ui->pushButton_giveup->setEnabled(false);
        ui->pushButton_askfordraw->setEnabled(false);
        win_white();
        return;
    }
    else if (order_now.order_kind == BlackWin)//黑胜
    {
        kill_timer(timerid);
        ui->pushButton_giveup->setEnabled(false);
        ui->pushButton_askfordraw->setEnabled(false);
        win_black();
        return;
    }

    else if (order_now.order_kind == Step)//走子模拟
    {
        chest->turn = order_now.turn;
        haveAskedForDraw = false;
        order_now.copyPiece(chest->piece);
        ui->lcdNumber->display(30);
        if (chest->turn == White)
            ui->label_turn->setText("White's turn");
        else
            ui->label_turn->setText("Black's turn");
        repaint();

        bool have_king = false;
        for (int i = 1; i <= 8; i++)
            for (int j = 1; j <= 8; j++)
                if (chest->piece[i][j] == chest->self_party && chest->piece[i][j] == King)
                {
                    have_king = true;
                    break;
                }

        if (!have_king)//王被吃了，输棋
        {
            Order myorder;
            if (chest->self_party == White)
            {
                myorder.order_kind = BlackWin;
                sendMessage(myorder);
                doOrder(myorder);
            }
            else {
                myorder.order_kind = WhiteWin;
                sendMessage(myorder);
                doOrder(myorder);
            }
            return;
        }

        if (chest->forceDraw())
        {
            if (chest->inDanger(chest->piece))//将死
            {
                Order order_tmp;
                order_tmp.order_kind = (chest->self_party == White) ? BlackWin : WhiteWin;
                sendMessage(order_tmp);
                doOrder(order_tmp);
            }
            else {//逼和
                Order order_tmp;
                order_tmp.order_kind = Draw;
                sendMessage(order_tmp);
                doOrder(order_tmp);
            }
        }
        return;
    }

    else if (order_now.order_kind == Ready)//准备
    {
        if (_ready)
        {
            _start = true;
            Order order_tmp;
            order_tmp.order_kind = Start;
            ui->label_turn->setText("White's turn");
            sendMessage(order_tmp);
            timerid = startTimer(1000);
            ui->lcdNumber->display(30);
            repaint();
        }
        _ready = true;
        return;
    }
    else if (order_now.order_kind == Start)//正式开始对局
    {
        _start = true;
        timerid = startTimer(1000);
        ui->lcdNumber->display(30);
        ui->label_turn->setText("White's turn");
        repaint();
        return;
    }

    else if (order_now.order_kind == TimeOut)//超时
    {
        ui->pushButton_giveup->setEnabled(false);
        ui->pushButton_askfordraw->setEnabled(false);
        kill_timer(timerid);
        ui->pushButton_giveup->setEnabled(false);
        ui->pushButton_askfordraw->setEnabled(false);
        QMessageBox::information(this, "Tips", "You win because of your oponent's time-out! ", QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    else if (order_now.order_kind == DrawAsk)//求和
    {
        int result = QMessageBox::No;
        result = QMessageBox::question(this, "Draw ask", "Your opponent asks for a draw. Will you accept? ");
        Order order_tmp;
        if (result == QMessageBox::No)
        {
            order_tmp.order_kind = RejectDraw;
            sendMessage(order_tmp);
        }
        else
        {
            kill_timer(timerid);
            ui->pushButton_giveup->setEnabled(false);
            ui->pushButton_askfordraw->setEnabled(false);
            order_tmp.order_kind = AcceptDraw;
            sendMessage(order_tmp);
            QMessageBox::information(this, "Tips", "You have reached a draw! ", QMessageBox::Ok, QMessageBox::Ok);
        }
        return;
    }
    else if (order_now.order_kind == RejectDraw)//拒绝求和请求
    {
        QMessageBox::information(this, "Tips", "Your opponent rejected your draw ask. ", QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    else if (order_now.order_kind == AcceptDraw)//同意求和请求
    {
        kill_timer(timerid);
        ui->pushButton_giveup->setEnabled(false);
        ui->pushButton_askfordraw->setEnabled(false);
        QMessageBox::information(this, "Tips", "Your opponent consented your draw ask! ", QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    else if (order_now.order_kind == Draw)
    {
        kill_timer(timerid);
        ui->pushButton_giveup->setEnabled(false);
        ui->pushButton_askfordraw->setEnabled(false);
        QMessageBox::information(this, "Tips", "You have reached a draw. ", QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    else if (order_now.order_kind == LoadAsk)//请求读档
    {
        kill_timer(timerid);
        chest_load = new Chest;
        chest_load->turn = order_now.turn;
        order_now.copyPiece(chest_load->piece);
        Chest chest_tmp = *chest;
        *chest = *chest_load;
        *chest_load = chest_tmp;

        if (chest->turn == White)
            ui->label_turn->setText("White's turn");
        else
            ui->label_turn->setText("Black's turn");
        repaint();

        int result = QMessageBox::No;
        result = QMessageBox::question(this, "Load file request", "Your opponent requests to load file as the boarder shows. \nWill you consent? ");
        if (result == QMessageBox::Yes)
        {
            ui->lcdNumber->display(30);
            Order order_tmp;
            order_tmp.order_kind = LoadConsent;
            sendMessage(order_tmp);
        }
        else {
            *chest = *chest_load;
            repaint();
            Order order_tmp;
            order_tmp.order_kind = LoadReject;
            sendMessage(order_tmp);
        }
        timerid = startTimer(1000);
        delete chest_load;
        chest_load = nullptr;
        return;
    }
    else if (order_now.order_kind == LoadConsent)//同意读档
    {
        ui->lcdNumber->display(30);
        timerid = startTimer(1000);
        delete chest_load;
        chest_load = nullptr;
        return;
    }
    else if (order_now.order_kind == LoadReject)//拒绝读档
    {
        *chest = *chest_load;
        timerid = startTimer(1000);
        delete chest_load;
        chest_load = nullptr;
        repaint();
        QMessageBox::information(this, "Tips", "Your opponent rejected to load the file. ");
        return;
    }
}

void MainWindow::win_black()
{
    QMessageBox::information(this, "Tips", "Black wins! ", QMessageBox::Ok, QMessageBox::Ok);
    kill_timer(timerid);
}

void MainWindow::win_white()
{
    QMessageBox::information(this, "Tips", "White wins! ", QMessageBox::Ok, QMessageBox::Ok);
    kill_timer(timerid);
}

void MainWindow::Adraw()
{
    QMessageBox::information(this, "Tips", "You have reached a draw! ", QMessageBox::Ok, QMessageBox::Ok);
    kill_timer(timerid);
}

void MainWindow::timerOut()
{
    ui->pushButton_giveup->setEnabled(false);
    ui->pushButton_askfordraw->setEnabled(false);
    kill_timer(timerid);
    Order order_tmp;
    order_tmp.order_kind = TimeOut;
    sendMessage(order_tmp);
    QMessageBox::information(this, "Tips", "You lose because of time-out! ", QMessageBox::Ok, QMessageBox::Ok);
}

bool MainWindow::can_go(Piece piece_now[9][9], Position from, Position target)
{
    Piece piece_from = piece_now[from._x][from._y], piece_target = piece_now[target._x][target._y];
    if (piece_from == None)
        return false;
    Party self_party = piece_now[from._x][from._y].party;
    Party enemy_party = (self_party == White) ? Black : White;

    if (from == target || (piece_target.kind != None && piece_from.party == piece_target.party))
        return false;

    if (piece_from == King)//要判断王车易位
    {
        if (target._y == from._y && abs(target._x - from._x) == 2)
        {
            if (from._x != 5)
                return false;
            if (piece_from == White)//白方王车易位
            {
                if (from._y != 1)
                    return false;
                if (target._x == from._x + 2 && piece_now[8][1] == Rook && piece_now[8][1] == White)
                {
                    if (piece_now[6][1] != None || piece_now[7][1] != None)
                        return false;
                    for (int k = 5; k <= 7; k++)
                        for (int i = 1; i <= 8; i++)
                            for (int j = 1; j <= 8; j++)
                            {
                                if (piece_now[i][j] == White || piece_now[i][j] == None)
                                    continue;
                                if (can_go(piece_now, Position(i, j), Position(k, 1)))
                                    return false;
                            }
                    return true;
                }
                else if (target._x == from._x - 2 && piece_now[1][1] == Rook && piece_now[1][1] == White)
                {
                    if (piece_now[2][1] != None || piece_now[3][1] != None || piece_now[4][1] != None)
                        return false;
                    for (int k = 3; k <= 5; k++)
                        for (int i = 1; i <= 8; i++)
                            for (int j = 1; j <= 8; j++)
                            {
                                if (piece_now[i][j] == White || piece_now[i][j] == None)
                                    continue;
                                if (can_go(piece_now, Position(i, j), Position(k, 1)))
                                    return false;
                            }
                    return true;
                }
                return false;
            }
            else {//黑色方王车易位
                if (from._y != 8)
                    return false;
                if (target._x == from._x + 2 && piece_now[8][8] == Rook && piece_now[8][8] == Black)
                {
                    if (piece_now[6][8] != None || piece_now[7][8] != None)
                        return false;
                    for (int k = 5; k <= 7; k++)
                        for (int i = 1; i <= 8; i++)
                            for (int j = 1; j <= 8; j++)
                            {
                                if (piece_now[i][j] == Black || piece_now[i][j] == None)
                                    continue;
                                if (can_go(piece_now, Position(i, j), Position(k, 8)))
                                    return false;
                            }
                    return true;
                }
                else if (target._x == from._x - 2 && piece_now[1][8] == Rook && piece_now[1][8] == Black)
                {
                    if (piece_now[4][8] != None || piece_now[3][8] != None || piece_now[2][8] != None)
                        return false;
                    for (int k = 3; k <= 5; k++)
                        for (int i = 1; i <= 8; i++)
                            for (int j = 1; j <= 8; j++)
                            {
                                if (piece_now[i][j] == Black || piece_now[i][j] == None)
                                    continue;
                                if (can_go(piece_now, Position(i, j), Position(k, 8)))
                                    return false;
                            }
                    return true;
                }
                return false;
            }
        }
        if (abs(target._x - from._x) > 1 || abs(target._y - from._y) > 1)
            return false;
        return true;
    }
    else if (piece_from == Queen)
    {
        //竖着走
        if (from._x == target._x)
        {
            if (from._y < target._y)
            {
                for (int i = from._y + 1; i < target._y; i++)
                    if (piece_now[from._x][i] != None)
                        return false;
                return true;
            }
            else {
                for (int i = from._y - 1; i > target._y; i--)
                    if (piece_now[from._x][i] != None)
                        return false;
                return true;
            }
        }

        //横着走
        if (from._y == target._y)
        {
            if (from._x < target._x)
            {
                for (int i = from._x + 1; i < target._x; i++)
                    if (piece_now[i][from._y] != None)
                        return false;
                return true;
            }
            else {
                for (int i = from._x - 1; i > target._x; i--)
                    if (piece_now[i][from._y] != None)
                        return false;
                return true;
            }
        }

        //剩余的是斜着走
        if (abs(from._x - target._x) != abs(from._y - target._y))
            return false;
        if (target._x > from._x && target._y > from._y)
        {
            for (int i = 1; i < target._x - from._x; i++)
                if (piece_now[i + from._x][i + from._y] != None)
                    return false;
            return true;
        }
        else if (target._x > from._x && target._y < from._y)
        {
            for (int i = 1; i < target._x - from._x; i++)
                if (piece_now[i + from._x][from._y - i] != None)
                    return false;
            return true;
        }
        else if (target._x < from._x && target._y > from._y)
        {
            for (int i = 1; i < from._x - target._x; i++)
                if (piece_now[from._x - i][i + from._y] != None)
                    return false;
            return true;
        }
        else if (target._x < from._x && target._y < from._y)
        {
            for (int i = 1; i < from._x - target._x; i++)
                if (piece_now[from._x - i][from._y - i] != None)
                    return false;
            return true;
        }
        return false;
    }
    else if (piece_from == Bishop)
    {
        if (abs(from._x - target._x) != abs(from._y - target._y))
            return false;
        if (target._x > from._x && target._y > from._y)
        {
            for (int i = 1; i < target._x - from._x; i++)
                if (piece_now[i + from._x][i + from._y] != None)
                    return false;
            return true;
        }
        if (target._x > from._x && target._y < from._y)
        {
            for (int i = 1; i < target._x - from._x; i++)
                if (piece_now[i + from._x][from._y - i] != None)
                    return false;
            return true;
        }
        if (target._x < from._x && target._y > from._y)
        {
            for (int i = 1; i < from._x - target._x; i++)
                if (piece_now[from._x - i][i + from._y] != None)
                    return false;
            return true;
        }
        if (target._x < from._x && target._y < from._y)
        {
            for (int i = 1; i < from._x - target._x; i++)
                if (piece_now[from._x - i][from._y - i] != None)
                    return false;
            return true;
        }
    }
    else if (piece_from == Knight)
    {
        if (abs(from._x - target._x) == 2 && abs(from._y - target._y) == 1)
            return true;
        if (abs(from._x - target._x) == 1 && abs(from._y - target._y) == 2)
            return true;
        return false;
    }
    else if (piece_from == Rook)
    {
        //竖着走
        if (from._x == target._x)
        {
            if (from._y < target._y)
            {
                for (int i = from._y + 1; i < target._y; i++)
                    if (piece_now[from._x][i] != None)
                        return false;
                return true;
            }
            else {
                for (int i = from._y - 1; i > target._y; i--)
                    if (piece_now[from._x][i] != None)
                        return false;
                return true;
            }
        }

        //横着走
        if (from._y == target._y)
        {
            if (from._x < target._x)
            {
                for (int i = from._x + 1; i < target._x; i++)
                    if (piece_now[i][from._y] != None)
                        return false;
                return true;
            }
            else {
                for (int i = from._x - 1; i > target._x; i--)
                    if (piece_now[i][from._y] != None)
                        return false;
                return true;
            }
        }
        return false;
    }
    else if (piece_from == Pawn)//要被王车易位时判断不被将军时调用
    {
        if (self_party == White)
        {
            if (from._x == target._x && from._y == 2 && target._y == 4 &&
                    piece_target == None && piece_now[target._x][3] == None)
                return true;
            if (from._x == target._x && target._y - from._y == 1 && piece_target == None)
                return true;
            if (abs(from._x - target._x) == 1 && target._y - from._y == 1 &&
                    piece_target != None && piece_target == enemy_party)
                return true;
            return false;
        }
        if (self_party == Black)
        {
            if (from._x == target._x && from._y == 7 && target._y == 5 &&
                    piece_target == None && piece_now[target._x][6] == None)
                return true;
            if (from._x == target._x && target._y - from._y == -1 && piece_target == None)
                return true;
            if (abs(from._x - target._x) == 1 && from._y - target._y == 1 &&
                    piece_target != None && piece_target == enemy_party)
                return true;
            return false;
        }
    }
}

void MainWindow::calculateAvailable(Piece piece_now[9][9])
{
    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
            available[i][j] = can_go(piece_now, Position(hold_position._x, hold_position._y), Position(i, j));
}

void MainWindow::kill_timer(int &timerid)
{
    if (timerid < 0)
        return;
    killTimer(timerid);
    timerid = -100;
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QColor close_to_white, close_to_black;
    close_to_black.setRgb(185, 131, 99);
    close_to_white.setRgb(240, 218, 181);

    QTransform transform;

    QPen bound_pen(Qt::black);
    bound_pen.setWidth(2);

    QPainter painter(this);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    //画棋盘底色
    transform.translate(x_start, y_start);
    painter.setTransform(transform);
    painter.setPen(Qt::NoPen);
    painter.setBrush(close_to_white);

    painter.drawRect(0, 0, width, width);
    painter.restore();
    painter.save();
    transform.reset();


    //为了与数组下标匹配正常，重新转换坐标
    transform.translate(x_start - grid, y_start - grid);
    painter.setPen(Qt::NoPen);
    painter.setBrush(close_to_black);
    painter.setTransform(transform);

    painter.setPen(Qt::NoPen);
    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
        {
            if ((i + j) & 1)
                continue;
            int j_transformed = 9 - j;
            painter.drawRect(i * grid, j_transformed * grid, grid, grid);
        }



    //显示选中棋子的高亮和能够走的格子的高亮
    QColor black_background;
    black_background.setRgb(0, 0, 0, 50);

    QColor blue_background;
    blue_background.setRgb(0, 0, 255, 50);

    if (hold)
    {
        painter.setBrush(black_background);
        painter.drawRect(hold_position._x * grid, (9 - hold_position._y) * grid, grid, grid);

        painter.setBrush(blue_background);
        for (int i = 1; i <= 8; i++)
            for (int j = 1; j <= 8; j++)
                if (available[i][j])
                {
                    int j_transformed = 9 - j;
                    painter.drawRect(i * grid, j_transformed * grid, grid, grid);
                }
    }


    //画棋子
    if (_start)
    {
        for (int i = 1; i <= 8; i++)
            for (int j = 1; j <= 8; j++)
            {
                int j_transformed = 9 - j;
                if (chest->piece[i][j] != None)
                {
                    painter.drawPixmap(i * grid, j_transformed * grid, grid, grid, picture[chest->piece[i][j].party][chest->piece[i][j].kind]);
                }
            }
    }


    //画棋盘最外面的边框
    transform.reset();
    transform.translate(x_start, y_start);

    painter.restore();
    painter.setTransform(transform);
    painter.setPen(bound_pen);
    painter.setBrush(Qt::NoBrush);

    painter.drawRect(0, 0, width, width);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (chest->turn != chest->self_party || !_start)
        return;

    int x = int((event->x() - x_start) / grid) + 1, y = 8 - int((event->y() - y_start) / grid);
    if (x < 1 || x > 8 || y < 1 || y > 8)
        return;

    if (!hold)//没捏着棋子
    {
        if (chest->piece[x][y] == None || chest->piece[x][y] != chest->self_party)
            return;
        QSound::play(":/soundeffect/soundeffect/pieceUp.wav");
        hold = true;
        hold_position.SetValue(x, y);
        calculateAvailable(chest->piece);
        repaint();
        return;
    }
    else {//捏着棋子
        hold = false;
        if (!available[x][y] || (x == hold_position._x && y == hold_position._y))
        {
            for (int i = 0; i < 9; i++)
                for (int j = 0; j < 9; j++)
                    available[i][j] = false;
            repaint();
            return;
        }
        QSound::play(":/soundeffect/soundeffect/pieceDown.wav");
        chest->piece[x][y] = chest->piece[hold_position._x][hold_position._y];
        chest->piece[hold_position._x][hold_position._y] = None;

        //特殊情况判断
        if (chest->piece[x][y] == King)//王车易位
        {
            if (x - hold_position._x == 2)
            {
                chest->piece[hold_position._x][y] = None;
                chest->piece[x - 1][y] = chest->piece[8][y];
                chest->piece[8][y] = None;
            }
            else if (hold_position._x - x == 2)
            {
                chest->piece[hold_position._x][y] = None;
                chest->piece[x + 1][y] = chest->piece[1][y];
                chest->piece[1][y] = None;
            }
        }
        else if (chest->piece[x][y] == Pawn && (y == 1 || y == 8))//兵的升变
        {
            repaint();
            PromotionDialog prodia(this);
            connect(&prodia, &PromotionDialog::setPromotion, [=](int k){
                this->promotionKind = Kind(k);
            });
            prodia.exec();
            chest->piece[x][y] = promotionKind;
        }

        //转变走子方
        chest->turn = (chest->turn == White) ? Black : White;
        if (chest->turn == White)
            ui->label_turn->setText("White's turn");
        else
            ui->label_turn->setText("Black's turn");

        repaint();
        Order order_tmp;
        order_tmp.order_kind = Step;
        order_tmp.turn = chest->turn;

        order_tmp.setPiece(chest->piece);
        sendMessage(order_tmp);
        ui->lcdNumber->display(30);
    }
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == timerid)
    {
        if (ui->lcdNumber->intValue() == 0)
            return;
        ui->lcdNumber->display(ui->lcdNumber->intValue() - 1);
        if (ui->lcdNumber->intValue() <= 5)
            QSound::play(":/soundeffect/soundeffect/countdown.wav");
        if (ui->lcdNumber->intValue() == 0 && chest->turn == chest->self_party)
            timerOut();
    }
}

void MainWindow::initServer()
{
    if (listenSocket)
    {
        disconnect(listenSocket,SIGNAL(newConnection()),this,SLOT(acceptConnection()));
        listenSocket->close();
        delete listenSocket;
    }
    this->listenSocket =new QTcpServer;

    QString localHostName = QHostInfo::localHostName();
    QHostInfo info = QHostInfo::fromName(localHostName);
    QHostAddress address;
    for (int i = 0; i < info.addresses().length(); i++)
    {
        if (info.addresses()[i].protocol() == QAbstractSocket::IPv4Protocol)
        {
            address = info.addresses()[i];
            break;
        }
    }

    lisDia = new ListeningDialog(this);
    lisDia->father = this;
    lisDia->setIP(address);
    lisDia->exec();

    ui->label_selfparty->setText("White");
}

void MainWindow::acceptConnection()
{
    lisDia->close();
    readWriteSocket = listenSocket->nextPendingConnection();

    qDebug() << readWriteSocket->peerAddress().toString();

    connect(readWriteSocket,SIGNAL(readyRead()),this,SLOT(receiveMessage()));
    connect(readWriteSocket, SIGNAL(disconnected()),readWriteSocket, SLOT(deleteLater()));
    listenSocket->close();

    ui->pushButton_ready->setEnabled(true);
    ui->pushButton_askfordraw->setEnabled(true);
    ui->pushButton_giveup->setEnabled(true);
}

void MainWindow::receiveMessage()
{
    QByteArray array;
    array.clear();
    array = readWriteSocket->readAll();
    Order order_now;

    if (!array.isEmpty())
    {
        char * src = array.data();
//        qFromBigEndian(src);
        memcpy(&order_now, src, sizeof(struct Order));
        doOrder(order_now);
    }
}

void MainWindow::sendMessage(const Order &order_now)
{
    QByteArray array;
    array.clear();
    array.resize(sizeof(struct Order));

    memcpy(array.data(), &order_now, sizeof(struct Order));
//    char * src = array.data();
//    qToBigEndian(src);

    readWriteSocket->write(array);
}

void MainWindow::setPromotion(int k)
{
    promotionKind = Kind(k);
}

void MainWindow::loadFile()
{
    QString filename = "";
    filename = QFileDialog::getOpenFileName(this, "Load", "./", "*.txt");
    if (filename == "")
        return;
    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    QString text = file.readAll();
    QStringList list = text.split('\n');
    if (list[list.length() - 1] == "")
        list.removeAt(list.length() - 1);
    int white_start = -1, white_end = -1, black_start = -1, black_end = -1;
    for (int i = 0; i < list.length(); i++)
    {
        list[i].replace("\r", "");
        if (list[i] == "white")
            white_start = i + 1;
        else if (list[i] == "black")
            black_start = i + 1;
    }

    qDebug() << list;

    if (white_start < black_start)
    {
        white_end = black_start - 1;
        black_end = list.length();
    }
    else {
        black_end = white_start - 1;
        white_end = list.length();
    }

    //[start, end)
    chest_load = new Chest;
    if (white_start < black_start)
        chest_load->turn = White;
    else
        chest_load->turn = Black;

    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
            chest_load->piece[i][j] = None;
    //先遍历白色棋子
    for (int i = white_start; i < white_end; i++)
    {
        QStringList now = list[i].split(' ');

        qDebug() << now;

        Kind kind = Kind(pieceNumber[now[0]]);
        for (int j = 2; j < now.length(); j++)
        {
            int x = now[j][0].toLatin1() - 96, y = now[j][1].toLatin1() - 48;
            chest_load->piece[x][y] = kind;
            chest_load->piece[x][y] = White;
        }
    }
    //再遍历黑色棋子
    for (int i = black_start; i < black_end; i++)
    {
        QStringList now = list[i].split(' ');
        Kind kind = Kind(pieceNumber[now[0]]);
        for (int j = 2; j < now.length(); j++)
        {
            int x = now[j][0].toLatin1() - 96, y = now[j][1].toLatin1() - 48;
            chest_load->piece[x][y] = kind;
            chest_load->piece[x][y] = Black;
        }
    }

    //暂停计时，棋盘复制，刷新
    kill_timer(timerid);
    Chest chest_tmp = *chest;
    *chest = *chest_load;
    *chest_load = chest_tmp;
    if (chest->turn == White)
        ui->label_turn->setText("White's turn");
    else
        ui->label_turn->setText("Black's turn");
    repaint();

    Order order_tmp;
    order_tmp.order_kind = LoadAsk;
    order_tmp.turn = chest->turn;
    order_tmp.setPiece(chest->piece);
    sendMessage(order_tmp);
    QMessageBox::information(this, "Tips", "Asking opponent if he consents to load the file. ");
}

void MainWindow::saveFile()
{
    QString filename = "";
    filename = QFileDialog::getSaveFileName(this, "Save", "./", "*.txt");
    if (filename == "")
        return;
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);

    int num[7];
    Position position[7][8];

    //等待走子的一方
    if (chest->turn == White)
        out << "white\n";
    else
        out << "black\n";
    for (int i = 0; i < 7; i++)
        num[i] = 0;
    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
        {
            if (chest->piece[i][j].kind == None || chest->piece[i][j].party != chest->turn)
                continue;
            position[int(chest->piece[i][j].kind)][num[int(chest->piece[i][j].kind)]++].SetValue(i, j);
        }
    for (int i = 1; i < 7; i++)
    {
        if (num[i] == 0)
            continue;
        out << pieceName[i] << ' ' << num[i];
        for (int j = 0; j < num[i]; j++)
        {
            out << ' ' << char(96 + position[i][j]._x) << position[i][j]._y;
        }
        out << '\n';
    }

    //后走子的一方
    if (chest->turn == Black)
        out << "white\n";
    else
        out << "black\n";
    for (int i = 0; i < 7; i++)
        num[i] = 0;
    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
        {
            if (chest->piece[i][j] == None || chest->piece[i][j] == chest->turn)
                continue;
            position[int(chest->piece[i][j].kind)][num[int(chest->piece[i][j].kind)]++].SetValue(i, j);
        }
    for (int i = 1; i < 7; i++)
    {
        if (num[i] == 0)
            continue;
        out << pieceName[i] << ' ' << num[i];
        for (int j = 0; j < num[i]; j++)
        {
            out << ' ' << char(96 + position[i][j]._x) << position[i][j]._y;
        }
        out << '\n';
    }

    out.flush();
    file.close();
}

void MainWindow::cancelListening()
{
    qDebug() << "cancel listening";
    lisDia->close();

    if (listenSocket)
    {
        disconnect(listenSocket,SIGNAL(newConnection()),this,SLOT(acceptConnection()));
        listenSocket->close();
    }
}

void MainWindow::connectHost()
{
    ListeningDialog ld(this);

    this->readWriteSocket = new QTcpSocket;
    connectingDialog cd(this);
    QString ip;
    cd.ip = &ip;
    cd.exec();

    this->readWriteSocket->connectToHost(QHostAddress(ip),7633);

    if (!readWriteSocket->waitForConnected(2000))
    {
        readWriteSocket->close();
        QMessageBox::critical(this, "Error", "Fail to connect to server. ");
    }

    connect(readWriteSocket,SIGNAL(readyRead()),this,SLOT(receiveMessage()));
    connect(readWriteSocket, SIGNAL(disconnected()),readWriteSocket, SLOT(deleteLater()));

    chest->self_party = Black;
    ui->label_selfparty->setText("Black");

    ui->pushButton_ready->setEnabled(true);
    ui->pushButton_askfordraw->setEnabled(true);
    ui->pushButton_giveup->setEnabled(true);
}
