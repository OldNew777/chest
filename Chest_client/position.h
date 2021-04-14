#ifndef POSITION_H
#define POSITION_H

#include <cmath>

class Position
{
public:
    int _x, _y;

    Position(){}
    Position(const int &x, const int &y)
    {
        _x = x;
        _y = y;
    }
    Position(const Position& b)
    {
        _x = b._x;
        _y = b._y;
    }

    Position& operator=(const Position& b)
    {
        _x = b._x;
        _y = b._y;
        return *this;
    }
    bool operator==(const Position& b)
    {
        return (_x == b._x && _y == b._y);
    }
    bool operator!=(const Position& b)
    {
        return (_x != b._x || _y != b._y);
    }

    void SetValue(const int &x, const int &y)
    {
        _x = x;
        _y = y;
    }
};

#endif // POSITION_H
