#ifndef MIRROR_H
#define MIRROR_H

#include "points.h"

class Mirror
{
public:
    Mirror(Points st, Points end, double rad);

    bool   Crossing(const Mirror& other) const;
    double Dist() const;
    double getRad() const;
    Points CircleCenter() const;
    Points getStart() const;
    Points getEnd() const;
    bool   PointOnArc(const Points& p, const Points& center) const;

private:
    Points start_, end_;
    double rad_;
};

class ConvexMirror : public Mirror
{
};

class ConcaveMirror : public Mirror
{
};

class PlaneMirror : public Mirror
{
};

#endif // MIRROR_H
