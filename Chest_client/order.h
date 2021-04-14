#ifndef ORDER_H
#define ORDER_H

#include "piece.h"
#include "position.h"


enum OrderKind
{
    GiveUp = 1,

    Step = 2,

    WhiteWin = 3,
    BlackWin = 4,

    Ready = 5,       //准备
    Start = 6,

    Save = 7,

    DrawAsk = 8,
    AcceptDraw = 9,
    RejectDraw = 10,
    Draw = 11,

    TimeOut = 12,

    LoadAsk = 13,
    LoadConsent = 14,
    LoadReject = 15
};


struct Order
{
public:
    Party turn;
    OrderKind order_kind = Step;

    Piece piece[9][9];

    void setPiece(Piece p[9][9])
    {
        for (int i = 1; i <= 8; i++)
            for (int j = 1; j <= 8; j++)
                piece[i][j] = p[i][j];
    }
    void copyPiece(Piece p[9][9])
    {
        for (int i = 1; i <= 8; i++)
            for (int j = 1; j <= 8; j++)
                p[i][j] = piece[i][j];
    }
};

#endif // ORDER_H
