#ifndef POINTS_H
#define POINTS_H

#include <string>
#include <iomanip>
#include <sstream>

class Points
{
public:
    Points(double x = 0.0, double y = 0.0);
    double getX() const;
    double getY() const;
    double Dist(const Points& other) const;
    std::string Cords() const;

private:
    double x_;
    double y_;
};


class Beam : public Points
{
public:
    Beam(double x = 0.0, double y = 0.0, double angle = 0.0);
    double getA() const;
    std::string Cords() const;

private:
    double angle_;
};

#endif // POINTS_H
