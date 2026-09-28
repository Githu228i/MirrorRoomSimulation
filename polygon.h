#ifndef POLYGON_H
#define POLYGON_H

#include <vector>
#include "mirror.h"

class Polygon
{
public:
    Polygon();

private:
    std::vector<Mirror> sides_;
    Beam beam_;
};

#endif // POLYGON_H
