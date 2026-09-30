#ifndef POLYGON_H
#define POLYGON_H

#include <vector>
#include <iostream>
#include <fstream>
#include "mirror.h"

class Polygon
{
public:
    Polygon(const std::vector<Mirror>& sides, Beam beam);

    void Push(const Mirror& other);
    void Del(int idx);
    void Write();

    bool Check();

    int Size();

private:
    std::vector<Mirror> sides_;
    Beam beam_;
};

#endif // POLYGON_H
