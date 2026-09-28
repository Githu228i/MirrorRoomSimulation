#ifndef POINTS_H
#define POINTS_H

class Points
{
public:
    Points();

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
