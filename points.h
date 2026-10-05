#pragma once
#include <string>
#include <sstream>
#include <iomanip>

class Points {
public:
    Points(double x = 0.0, double y = 0.0);
    virtual ~Points() = default;

    double getX() const;
    double getY() const;
    double Dist(const Points& other) const;
    virtual std::string Cords() const;

protected:
    double x_;
    double y_;
};

class Beam : public Points {
public:
    Beam(double x = 0.0, double y = 0.0, double angle = 0.0);

    double getA() const;
    std::string Cords() const override;

private:
    double angle_;
};
