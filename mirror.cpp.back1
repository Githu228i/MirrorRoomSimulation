#include "mirror.h"
#include <cmath>
#include <algorithm>

// ---------- Конструктор ----------
Mirror::Mirror(Points st, Points end, double rad) {
    start_ = st;
    end_   = end;
    rad_   = rad;
}

// ---------- Геттеры ----------
double Mirror::getRad() const   { return rad_; }
Points Mirror::getStart() const { return start_; }
Points Mirror::getEnd() const   { return end_; }

// ---------- Длина хорды линзы ----------
double Mirror::Dist() const {
    return start_.Dist(end_);
}

// ---------- Центр окружности, образующей линзу ----------
Points Mirror::CircleCenter() const {
    Points p1 = start_;
    Points p2 = end_;
    double r = rad_;

    double d = p1.Dist(p2);

    if (d < 1e-12) {
        return Points((p1.getX() + p2.getX()) / 2.0,
                      (p1.getY() + p2.getY()) / 2.0);
    }

    if (std::abs(r) < d / 2.0) {
        return Points((p1.getX() + p2.getX()) / 2.0,
                      (p1.getY() + p2.getY()) / 2.0);
    }

    Points mid((p1.getX() + p2.getX()) / 2.0,
               (p1.getY() + p2.getY()) / 2.0);

    double h = std::sqrt(r * r - (d / 2.0) * (d / 2.0));

    double dx = (p2.getX() - p1.getX()) / d;
    double dy = (p2.getY() - p1.getY()) / d;

    double perpX = -dy;
    double perpY =  dx;

    double sign = (r > 0) ? 1.0 : -1.0;

    return Points(mid.getX() + sign * h * perpX,
                  mid.getY() + sign * h * perpY);
}

// ---------- Лежит ли точка на дуге линзы ----------
bool Mirror::PointOnArc(const Points& p, const Points& center) const {
    double distToCenter = p.Dist(center);
    if (std::abs(distToCenter - std::abs(rad_)) > 1e-6)
        return false;

    double dx = end_.getX() - start_.getX();
    double dy = end_.getY() - start_.getY();
    double len2 = dx * dx + dy * dy;
    if (len2 < 1e-12) return false;

    double t = ((p.getX() - start_.getX()) * dx +
                (p.getY() - start_.getY()) * dy) / len2;

    return (t >= -1e-6 && t <= 1.0 + 1e-6);
}

// ---------- Пересечение двух линз ----------
bool Mirror::Crossing(const Mirror& other) const {
    Points c1 = CircleCenter();
    Points c2 = other.CircleCenter();

    double r1 = std::abs(rad_);
    double r2 = std::abs(other.rad_);
    double d  = c1.Dist(c2);

    if (d < 1e-12)
        return std::abs(r1 - r2) < 1e-9;

    if (d > r1 + r2 + 1e-9)            return false;
    if (d < std::abs(r1 - r2) - 1e-9)  return false;

    double a = (r1 * r1 - r2 * r2 + d * d) / (2.0 * d);
    double hSq = r1 * r1 - a * a;
    if (hSq < -1e-9) return false;
    double h = std::sqrt(std::max(0.0, hSq));

    double px = c1.getX() + a * (c2.getX() - c1.getX()) / d;
    double py = c1.getY() + a * (c2.getY() - c1.getY()) / d;

    Points inter1(px + h * (c2.getY() - c1.getY()) / d,
                  py - h * (c2.getX() - c1.getX()) / d);
    Points inter2(px - h * (c2.getY() - c1.getY()) / d,
                  py + h * (c2.getX() - c1.getX()) / d);

    if ((PointOnArc(inter1, c1) && other.PointOnArc(inter1, c2)) ||
        (PointOnArc(inter2, c1) && other.PointOnArc(inter2, c2)))
        return true;

    if (PointOnArc(other.getStart(), c1) && PointOnArc(other.getEnd(), c1))
        return true;
    if (other.PointOnArc(getStart(), c2) && other.PointOnArc(getEnd(), c2))
        return true;

    return false;
}

