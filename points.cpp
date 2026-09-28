#include "points.h"
#include <cmath>

Points::Points(double x, double y) : x_(x), y_(y) {}

double Points::getX() const { return x_; }
double Points::getY() const { return y_; }

double Points::Dist(const Points& other) const {
    double diffx = getX() - other.getX();
    double diffy = getY() - other.getY();
    return std::sqrt(diffx * diffx + diffy * diffy);
}

