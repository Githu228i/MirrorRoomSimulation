#ifndef POLYGON_H
#define POLYGON_H

#include <vector>
#include "mirror.h"

class Polygon
{
public:
    Polygon();
    friend class MainWindow;
    friend class FrontEndOperators;
    bool isExist();
private:
    std::vector<Mirror> sides_;
    Points first;
    bool isExist_ = false;
    Beam beam_;
};

#endif // POLYGON_H
