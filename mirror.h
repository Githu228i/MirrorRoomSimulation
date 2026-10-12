#ifndef MIRROR_H
#define MIRROR_H

#include "points.h"

#include <vector>

class QPainter;

// Зеркало: отрезок (rad == 0 или очень большой радиус) либо МЕНЬШАЯ дуга
// окружности радиуса |rad| между start и end (до полуокружности включительно).
// Знак rad выбирает сторону центра относительно хорды start->end:
//   rad > 0  - центр слева от хорды (выпуклое), rad < 0 - справа (вогнутое).
class Mirror
{
public:
    // Бросает std::invalid_argument при нулевой хорде или если |rad| < хорда/2.
    Mirror(Points st, Points end, double rad);
    Mirror(Points st, Points end);          // плоское (rad = 0)
    virtual ~Mirror() = default;

    // ---- Базовые геттеры ----
    double getRad() const;                  // радиус СО ЗНАКОМ (как в n2mirror)
    double getRadius() const;               // |радиус| (удобно для рисования)
    Points getStart() const;
    Points getEnd() const;
    double Dist() const;

    // ---- Геометрия ----
    virtual bool   isFlat() const;

    // Центр окружности со стороной, заданной знаком rad.
    virtual Points CircleCenter() const;
    // Центр окружности с явно заданной стороной
    // (leftSide = true -> слева от start -> end).
    Points CircleCenter(bool leftSide) const;

    virtual bool PointOnArc(const Points& p, const Points& center) const;

    // ---- Пересечения ----
    virtual std::vector<Points> Intersections(const Mirror& other) const;
    virtual bool Touches(const Mirror& other) const;
    virtual bool Crossing(const Mirror& other) const;

    // ---- Рисование ----
    virtual void Draw(
        QPainter& painter,
        bool insideOnLeft
        ) const = 0;

protected:
    Points start_;
    Points end_;
    double rad_;

    // Общие кирпичики для наследников.
    bool flatPointOnArc(const Points& p) const;
    bool arcPointOnArc(const Points& p, const Points& center) const;
};


class NotPlaneMirror : public Mirror
{
protected:
    NotPlaneMirror(
        Points st,
        Points end,
        double rad
        );
};


// Центр слева от хорды (rad > 0).
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


// Центр справа от хорды (rad < 0).
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


// Всегда отрезок.
class PlaneMirror : public Mirror
{
public:
    PlaneMirror(
        Points st,
        Points end
        );

    bool isFlat() const override;

    void Draw(
        QPainter& painter,
        bool insideOnLeft
        ) const override;
};

#endif // MIRROR_H
