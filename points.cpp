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

Beam::Beam(double x, double y, double angle): Points(x, y), angle_(angle) {}

double Beam::getA() const { return angle_; }

std::string Beam::Cords() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3)
        << getX() << " " << getY() << " " << angle_;
    return oss.str();
}

std::string Points::Cords() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3)
        << x_ << " " << y_;
    return oss.str();
}
