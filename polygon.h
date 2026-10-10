#ifndef POLYGON_H
#define POLYGON_H

#include <vector>
#include <memory>
#include "mirror.h"

class Polygon
{
public:
    Polygon();
    friend class MainWindow;
    friend class FrontEndOperators;
    bool isExist();
    bool isCycled();
private:
    std::vector<std::unique_ptr<Mirror>> sides_;
    Points first;
    bool isExist_ = false;
    bool isCycled_ = false;
    Beam beam_;
};

#endif // POLYGON_H
