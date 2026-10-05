#pragma once
#include "nmirror.h"
#include "points.h"
#include <vector>
#include <fstream>
#include <iostream>

class Polygon {
public:
    Polygon(const std::vector<Mirror>& sides, Beam beam);

    void Write() const;
    void Push(const Mirror& other);
    void Del(int idx);
    int  Size() const;

    // Контур замкнут и непрерывен, стороны не пересекаются и не касаются
    // нигде, кроме общих вершин соседних сторон.
    bool Check() const;

    // Трассировка луча, результат в beam.txt
    void TraceBeam(int maxBounces = 100);

private:
    std::vector<Mirror> sides_;
    Beam beam_;
};
