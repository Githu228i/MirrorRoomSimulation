#ifndef MIRROR_H
#define MIRROR_H

#include "points.h"

class QPainter;

class Mirror
{
public:
    Mirror(Points st, Points end);
    virtual ~Mirror() = default;

    bool Crossing(const Mirror& other) const;
    double Dist() const;

    virtual double getRad() const;

    Points CircleCenter(bool leftSide = true) const;

    Points getStart() const;
    Points getEnd() const;

    bool PointOnArc(
        const Points& p,
        const Points& center
        ) const;

    virtual void Draw(
        QPainter& painter,
        bool insideOnLeft
        ) const = 0;

protected:
    Points start_;
    Points end_;
};


class NotPlaneMirror : public Mirror
{
public:
    double getRad() const override;

protected:
    double rad_;

    NotPlaneMirror(
        Points st,
        Points end,
        double rad
        );
};


class ConvexMirror : public NotPlaneMirror
{
public:
    ConvexMirror(
        Points st,
        Points end,
        double rad
        );

    void Draw(
        QPainter& painter,
        bool insideOnLeft
        ) const override;
};


class ConcaveMirror : public NotPlaneMirror
{
public:
    ConcaveMirror(
        Points st,
        Points end,
        double rad
        );

    void Draw(
        QPainter& painter,
        bool insideOnLeft
        ) const override;
};


class PlaneMirror : public Mirror
{
public:
    PlaneMirror(
        Points st,
        Points end
        );

    void Draw(
        QPainter& painter,
        bool insideOnLeft
        ) const override;
};

#endif // MIRROR_H
