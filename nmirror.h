#ifndef MIRROR_H
#define MIRROR_H
#include "points.h"

class Mirror
{
public:
    Mirror(Points st, Points end, double rad);
    virtual ~Mirror() = default;

    // Виртуальные методы для поддержки полиморфизма
    virtual bool Crossing(const Mirror& other) const;
    double Dist() const;
    virtual double getRad() const;
    virtual Points CircleCenter() const;
    Points getStart() const;
    Points getEnd() const;
    virtual bool PointOnArc(const Points& p, const Points& center) const;

protected:
    Points start_, end_;
    double rad_;
};

class ConvexMirror : public Mirror
{
public:
    ConvexMirror(Points st, Points end, double rad);
    virtual bool Crossing(const Mirror& other) const override;
    virtual Points CircleCenter() const override;
    virtual bool PointOnArc(const Points& p, const Points& center) const override;
};

class ConcaveMirror : public Mirror
{
public:
    ConcaveMirror(Points st, Points end, double rad);
    virtual bool Crossing(const Mirror& other) const override;
    virtual Points CircleCenter() const override;
    virtual bool PointOnArc(const Points& p, const Points& center) const override;
};

#endif // MIRROR_H
