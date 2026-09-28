#ifndef POINTS_H
#define POINTS_H

class Points
{
public:
    Points(double x = 0.0, double y = 0.0);
    double getX() const;
    double getY() const;
    double Dist(const Points& other) const;

private:
    double x_;
    double y_;
};


class Beam : public Points
{
public:

private:
    double angle;
};

#endif // POINTS_H
