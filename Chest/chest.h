#ifndef CHEST_H
#define CHEST_H

#include "piece.h"
#include "position.h"

class MainWindow;
class Order;

class Chest
{
private:
    MainWindow * father;

    Piece piece[9][9];
    Party turn = White;
    Party self_party = White;

    friend class MainWindow;

public:
    Chest();
    bool forceDraw();
    bool inDanger(Piece piece_now[9][9]);

    Chest & operator=(const Chest & b);
};

#endif // CHEST_H
