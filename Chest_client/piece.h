#ifndef PIECE_H
#define PIECE_H

enum Party
{
    White = 1,
    Black = 2
};

enum Kind
{
    None = 0,

    King = 1,
    Queen = 2,
    Bishop = 3,
    Knight = 4,
    Rook = 5,
    Pawn = 6
};


class Piece
{
public:
    Kind kind = None;
    Party party = White;


    Piece(){}
    Piece(const Piece & b)
    {
        kind = b.kind;
        party = b.party;
    }

    Piece & operator=(const Piece & b)
    {
        kind = b.kind;
        party = b.party;
        return *this;
    }
    Piece & operator=(const Kind & b)
    {
        kind = b;
        return *this;
    }
    Piece & operator=(const Party & b)
    {
        party = b;
        return *this;
    }

    bool operator==(const Kind & b)
    {
        return (kind == b);
    }
    bool operator==(const Party & b)
    {
        return (party == b);
    }
    bool operator!=(const Kind & b)
    {
        return (kind != b);
    }
    bool operator!=(const Party & b)
    {
        return (party != b);
    }
};

#endif // PIECE_H
