#pragma once
#include "points.h"
#include <vector>

// Зеркало: отрезок (rad == 0 или очень большой радиус) либо МЕНЬШАЯ дуга
// окружности радиуса |rad| между start и end (до полуокружности включительно).
// Знак rad выбирает сторону центра относительно хорды start->end:
//   rad > 0  - центр слева от хорды (выпуклое), rad < 0 - справа (вогнутое).
class Mirror {
public:
    // Бросает std::invalid_argument при нулевой хорде или если |rad| < хорда/2.
    Mirror(Points st, Points end, double rad);
    virtual ~Mirror() = default;

    double getRad() const;
    Points getStart() const;
    Points getEnd() const;
    double Dist() const;

    // ---- Геометрия (переопределяется наследниками) ----
    virtual bool   isFlat() const;
    virtual Points CircleCenter() const;
    virtual bool   PointOnArc(const Points& p, const Points& center) const;

    // ---- Пересечения ----
    virtual std::vector<Points> Intersections(const Mirror& other) const;
    virtual bool Touches(const Mirror& other) const;
    virtual bool Crossing(const Mirror& other) const;

protected:
    Points start_;
    Points end_;
    double rad_;

    // Общие кирпичики для наследников.
    bool flatPointOnArc(const Points& p) const;                 // проверка для отрезка
    bool arcPointOnArc(const Points& p, const Points& center) const; // проверка для дуги
};

// Центр слева от хорды (rad > 0).
class ConvexMirror : public Mirror {
public:
    ConvexMirror(Points st, Points end, double rad);

    bool   isFlat() const override;
    Points CircleCenter() const override;
    bool   PointOnArc(const Points& p, const Points& center) const override;
    std::vector<Points> Intersections(const Mirror& other) const override;
};

// Центр справа от хорды (rad < 0).
class ConcaveMirror : public Mirror {
public:
    ConcaveMirror(Points st, Points end, double rad);

    bool   isFlat() const override;
    Points CircleCenter() const override;
    bool   PointOnArc(const Points& p, const Points& center) const override;
    std::vector<Points> Intersections(const Mirror& other) const override;
};

// Всегда отрезок.
class PlaneMirror : public Mirror {
public:
    PlaneMirror(Points st, Points end);

    bool   isFlat() const override;
    Points CircleCenter() const override;
    bool   PointOnArc(const Points& p, const Points& center) const override;
    std::vector<Points> Intersections(const Mirror& other) const override;
};
