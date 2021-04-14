#include "chest.h"
#include "mainwindow.h"
#include "order.h"


Chest::Chest()
{
    for (int i = 1; i <= 8; i += 7)
    {
        piece[1][i] = Rook;
        piece[8][i] = Rook;
        piece[2][i] = Knight;
        piece[7][i] = Knight;
        piece[3][i] = Bishop;
        piece[6][i] = Bishop;
        piece[4][i] = Queen;
        piece[5][i] = King;
    }

    for (int i = 1; i <= 8; i++)
    {
        piece[i][2] = Pawn;
        piece[i][7] = Pawn;

        piece[i][7] = Black;
        piece[i][8] = Black;
    }
}

bool Chest::forceDraw()
{
    Piece piece_tmp[9][9];

    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
            piece_tmp[i][j] = piece[i][j];

    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
        {
            if (piece[i][j] != self_party)
                continue;
            for (int p = 1; p <= 8; p++)
                for (int q = 1; q <= 8; q++)
                {
                    Piece tmp = piece[p][q];
                    if (!father->can_go(piece, Position(i, j), Position(p, q)))
                        continue;
                    piece_tmp[p][q] = piece_tmp[i][j];
                    piece_tmp[i][j] = None;
                    if (!inDanger(piece_tmp))
                            return false;
                    piece_tmp[i][j] = piece_tmp[p][q];
                    piece_tmp[p][q] = tmp;
                }
        }
    return true;
}

bool Chest::inDanger(Piece piece_now[9][9])
{
    int king_x = -1, king_y = -1;
    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
            if (piece_now[i][j] == self_party && piece_now[i][j] == King)
            {
                king_x = i;
                king_y = j;
            }
    if (king_x < 0 || king_y < 0)
        return true;
    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
            if (piece_now[i][j] != None && piece_now[i][j] != self_party && father->can_go(piece_now, Position(i, j), Position(king_x, king_y)))
                return true;
    return false;
}

Chest &Chest::operator=(const Chest &b)
{
    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
            piece[i][j] = b.piece[i][j];
    turn = b.turn;
}
