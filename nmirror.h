#pragma once
#include "points.h"
#include <vector>

// Зеркало: отрезок (rad == 0 или очень большой радиус) либо МЕНЬШАЯ дуга
// окружности радиуса |rad| между start и end (до полуокружности включительно).
// Знак rad выбирает сторону центра относительно хорды start->end:
//   rad > 0  - центр слева от хорды (выпуклое), rad < 0 - справа (вогнутое).
// Всё поведение определяется полями, поэтому наследники только задают знак,
// а Mirror безопасно хранить в std::vector<Mirror> по значению (без срезки логики).
class Mirror {
public:
    // Бросает std::invalid_argument при нулевой хорде или если |rad| < хорда/2.
    Mirror(Points st, Points end, double rad);
    virtual ~Mirror() = default;

    double getRad() const;
    Points getStart() const;
    Points getEnd() const;
    double Dist() const;

    bool   isFlat() const;
    Points CircleCenter() const;
    bool   PointOnArc(const Points& p, const Points& center) const;

    // Все точки пересечения (с учётом допусков, без дублей).
    std::vector<Points> Intersections(const Mirror& other) const;
    // Есть ли хоть одна общая точка (включая касание).
    bool Touches(const Mirror& other) const;
    // Пересечение, не сводящееся к одному общему концу (стык двух зеркал).
    bool Crossing(const Mirror& other) const;

protected:
    Points start_;
    Points end_;
    double rad_;
};

class ConvexMirror : public Mirror {
public:
    ConvexMirror(Points st, Points end, double rad);
};

class ConcaveMirror : public Mirror {
public:
    ConcaveMirror(Points st, Points end, double rad);
};

class PlaneMirror : public Mirror {
public:
    PlaneMirror(Points st, Points end);
};
