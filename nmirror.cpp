#include "nmirror.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <limits>

constexpr double EPS = 1e-6;
constexpr double PI = 3.14159265358979323846;

// ---------- Вспомогательные математические функции ----------
namespace {
    double NormalizeAngle(double a) {
        while (a < -PI) a += 2 * PI;
        while (a > PI) a -= 2 * PI;
        return a;
    }

    // Пересечение двух отрезков
    bool SegmentSegment(const Points& p1, const Points& p2, const Points& p3, const Points& p4) {
        double d1x = p2.getX() - p1.getX();
        double d1y = p2.getY() - p1.getY();
        double d2x = p4.getX() - p3.getX();
        double d2y = p4.getY() - p3.getY();

        double cross = d1x * d2y - d1y * d2x;

        if (std::abs(cross) < EPS) {
            double cross2 = (p3.getX() - p1.getX()) * d1y - (p3.getY() - p1.getY()) * d1x;
            if (std::abs(cross2) > EPS) return false;

            if (std::abs(d1x) > std::abs(d1y)) {
                double min1 = std::min(p1.getX(), p2.getX());
                double max1 = std::max(p1.getX(), p2.getX());
                double min2 = std::min(p3.getX(), p4.getX());
                double max2 = std::max(p3.getX(), p4.getX());
                return std::max(min1, min2) <= std::min(max1, max2) + EPS;
            } else {
                double min1 = std::min(p1.getY(), p2.getY());
                double max1 = std::max(p1.getY(), p2.getY());
                double min2 = std::min(p3.getY(), p4.getY());
                double max2 = std::max(p3.getY(), p4.getY());
                return std::max(min1, min2) <= std::min(max1, max2) + EPS;
            }
        }

        double t = ((p3.getX() - p1.getX()) * d2y - (p3.getY() - p1.getY()) * d2x) / cross;
        double u = ((p3.getX() - p1.getX()) * d1y - (p3.getY() - p1.getY()) * d1x) / cross;

        return (t >= -EPS && t <= 1.0 + EPS && u >= -EPS && u <= 1.0 + EPS);
    }

    // Пересечение отрезка с окружностью
    std::vector<Points> SegmentCircle(const Points& p1, const Points& p2, const Points& c, double r) {
        std::vector<Points> res;
        double dx = p2.getX() - p1.getX();
        double dy = p2.getY() - p1.getY();
        double fx = p1.getX() - c.getX();
        double fy = p1.getY() - c.getY();

        double A = dx*dx + dy*dy;
        double B = 2.0 * (fx*dx + fy*dy);
        double C = fx*fx + fy*fy - r*r;

        if (A < EPS) return res;

        double discriminant = B*B - 4*A*C;
        if (discriminant < -EPS) return res;
        if (discriminant < 0) discriminant = 0;

        double sqrtD = std::sqrt(discriminant);
        double t1 = (-B - sqrtD) / (2*A);
        double t2 = (-B + sqrtD) / (2*A);

        if (t1 >= -EPS && t1 <= 1.0 + EPS) {
            res.push_back(Points(p1.getX() + t1*dx, p1.getY() + t1*dy));
        }
        if (t2 >= -EPS && t2 <= 1.0 + EPS && std::abs(t1 - t2) > EPS) {
            res.push_back(Points(p1.getX() + t2*dx, p1.getY() + t2*dy));
        }
        return res;
    }

    // Пересечение двух окружностей
    std::vector<Points> CircleCircle(const Points& c1, double r1, const Points& c2, double r2) {
        std::vector<Points> res;
        double dx = c2.getX() - c1.getX();
        double dy = c2.getY() - c1.getY();
        double d2 = dx*dx + dy*dy;
        double d = std::sqrt(d2);

        if (d < EPS) return res; 
        if (d > r1 + r2 + EPS) return res; 
        if (d < std::abs(r1 - r2) - EPS) return res; 

        double a = (r1*r1 - r2*r2 + d2) / (2.0 * d);
        double hSq = r1*r1 - a*a;
        if (hSq < -EPS) return res;
        double h = (hSq < 0) ? 0.0 : std::sqrt(hSq);

        double px = c1.getX() + a * dx / d;
        double py = c1.getY() + a * dy / d;

        res.push_back(Points(px + h * dy / d, py - h * dx / d));
        if (h > EPS) {
            res.push_back(Points(px - h * dy / d, py + h * dx / d));
        }
        return res;
    }
} // namespace

// ==========================================
// ---------- Mirror (Прямое зеркало) -------
// ==========================================
Mirror::Mirror(Points st, Points end, double rad) : start_(st), end_(end), rad_(rad) {}

double Mirror::getRad() const { return rad_; }
Points Mirror::getStart() const { return start_; }
Points Mirror::getEnd() const { return end_; }
double Mirror::Dist() const { return start_.Dist(end_); }

Points Mirror::CircleCenter() const {
    // У прямого зеркала нет центра кривизны, возвращаем середину
    return Points((start_.getX() + end_.getX()) / 2.0,
                  (start_.getY() + end_.getY()) / 2.0);
}

bool Mirror::PointOnArc(const Points& p, const Points& center) const {
    // Проверка принадлежности точки ОТРЕЗКУ (прямому зеркалу)
    (void)center; // Игнорируем центр для прямой
    double dist = start_.Dist(end_);
    if (dist < EPS) return p.Dist(start_) < EPS;

    // Проверка коллинеарности
    double cross = (p.getX() - start_.getX()) * (end_.getY() - start_.getY()) -
                   (p.getY() - start_.getY()) * (end_.getX() - start_.getX());
    if (std::abs(cross) / dist > EPS) return false;

    // Проверка ограничивающего прямоугольника (bounding box)
    if (std::abs(end_.getX() - start_.getX()) > std::abs(end_.getY() - start_.getY())) {
        return (p.getX() >= std::min(start_.getX(), end_.getX()) - EPS &&
                p.getX() <= std::max(start_.getX(), end_.getX()) + EPS);
    } else {
        return (p.getY() >= std::min(start_.getY(), end_.getY()) - EPS &&
                p.getY() <= std::max(start_.getY(), end_.getY()) + EPS);
    }
}

bool Mirror::Crossing(const Mirror& other) const {
    // this - прямое зеркало. Пытаемся понять, чем является other.
    const ConvexMirror* cv = dynamic_cast<const ConvexMirror*>(&other);
    const ConcaveMirror* cc = dynamic_cast<const ConcaveMirror*>(&other);

    if (!cv && !cc) {
        // other - тоже прямое зеркало. Пересекаем два отрезка.
        return SegmentSegment(this->start_, this->end_, other.getStart(), other.getEnd());
    }

    // other - дуга. this - отрезок. Ищем пересечение отрезка с окружностью other.
    Points c = other.CircleCenter();
    double r = std::abs(other.getRad());
    
    auto intersections = SegmentCircle(this->start_, this->end_, c, r);
    for (const auto& pt : intersections) {
        // Виртуальный вызов other.PointOnArc сам выберет нужный метод (Convex или Concave)
        if (other.PointOnArc(pt, c)) return true;
    }
    return false;
}

// ==========================================
// ---------- ConvexMirror (Выпуклое) -------
// ==========================================
ConvexMirror::ConvexMirror(Points st, Points end, double rad) : Mirror(st, end, std::abs(rad)) {}

Points ConvexMirror::CircleCenter() const {
    Points p1 = start_;
    Points p2 = end_;
    double r = rad_;
    double d = p1.Dist(p2);

    if (d < EPS || r < d / 2.0 - EPS) {
        return Points((p1.getX() + p2.getX()) / 2.0, (p1.getY() + p2.getY()) / 2.0);
    }

    Points mid((p1.getX() + p2.getX()) / 2.0, (p1.getY() + p2.getY()) / 2.0);
    double h = std::sqrt(std::max(0.0, r * r - (d / 2.0) * (d / 2.0)));
    double dx = (p2.getX() - p1.getX()) / d;
    double dy = (p2.getY() - p1.getY()) / d;

    // Выбираем направление перпендикуляра "+" для выпуклости
    double perpX = -dy;
    double perpY =  dx;

    return Points(mid.getX() + h * perpX, mid.getY() + h * perpY);
}

bool ConvexMirror::PointOnArc(const Points& p, const Points& center) const {
    double distToCenter = p.Dist(center);
    if (std::abs(distToCenter - rad_) > 1e-6) return false;

    // Проверка через полярные углы
    double a1 = std::atan2(start_.getY() - center.getY(), start_.getX() - center.getX());
    double a2 = std::atan2(end_.getY() - center.getY(), end_.getX() - center.getX());
    double ap = std::atan2(p.getY() - center.getY(), p.getX() - center.getX());

    a1 = NormalizeAngle(a1);
    a2 = NormalizeAngle(a2);
    ap = NormalizeAngle(ap);

    double diff = NormalizeAngle(a2 - a1);
    double diffP = NormalizeAngle(ap - a1);

    if (diff >= 0) {
        return (diffP >= -1e-6 && diffP <= diff + 1e-6);
    } else {
        return (diffP <= 1e-6 && diffP >= diff - 1e-6);
    }
}

bool ConvexMirror::Crossing(const Mirror& other) const {
    const ConvexMirror* cv = dynamic_cast<const ConvexMirror*>(&other);
    const ConcaveMirror* cc = dynamic_cast<const ConcaveMirror*>(&other);

    if (!cv && !cc) {
        // other - прямое зеркало. Пересекаем дугу (this) с отрезком (other).
        Points c = this->CircleCenter();
        double r = this->rad_;
        auto intersections = SegmentCircle(other.getStart(), other.getEnd(), c, r);
        for (const auto& pt : intersections) {
            if (this->PointOnArc(pt, c)) return true;
        }
        return false;
    }

    // other - дуга. Пересекаем две дуги.
    Points c1 = this->CircleCenter();
    Points c2 = other.CircleCenter();
    double r1 = this->rad_;
    double r2 = std::abs(other.getRad());

    auto intersections = CircleCircle(c1, r1, c2, r2);
    for (const auto& pt : intersections) {
        if (this->PointOnArc(pt, c1) && other.PointOnArc(pt, c2)) {
            return true;
        }
    }
    return false;
}

// ==========================================
// ---------- ConcaveMirror (Вогнутое) ------
// ==========================================
ConcaveMirror::ConcaveMirror(Points st, Points end, double rad) : Mirror(st, end, std::abs(rad)) {}

Points ConcaveMirror::CircleCenter() const {
    Points p1 = start_;
    Points p2 = end_;
    double r = rad_;
    double d = p1.Dist(p2);

    if (d < EPS || r < d / 2.0 - EPS) {
        return Points((p1.getX() + p2.getX()) / 2.0, (p1.getY() + p2.getY()) / 2.0);
    }

    Points mid((p1.getX() + p2.getX()) / 2.0, (p1.getY() + p2.getY()) / 2.0);
    double h = std::sqrt(std::max(0.0, r * r - (d / 2.0) * (d / 2.0)));
    double dx = (p2.getX() - p1.getX()) / d;
    double dy = (p2.getY() - p1.getY()) / d;

    // Выбираем ПРОТИВОПОЛОЖНОЕ направление перпендикуляра "-" для вогнутости
    double perpX = -dy;
    double perpY =  dx;

    return Points(mid.getX() - h * perpX, mid.getY() - h * perpY);
}

bool ConcaveMirror::PointOnArc(const Points& p, const Points& center) const {
    // Логика проверки дуги идентична выпуклому зеркалу
    double distToCenter = p.Dist(center);
    if (std::abs(distToCenter - rad_) > 1e-6) return false;

    double a1 = std::atan2(start_.getY() - center.getY(), start_.getX() - center.getX());
    double a2 = std::atan2(end_.getY() - center.getY(), end_.getX() - center.getX());
    double ap = std::atan2(p.getY() - center.getY(), p.getX() - center.getX());

    a1 = NormalizeAngle(a1);
    a2 = NormalizeAngle(a2);
    ap = NormalizeAngle(ap);

    double diff = NormalizeAngle(a2 - a1);
    double diffP = NormalizeAngle(ap - a1);

    if (diff >= 0) {
        return (diffP >= -1e-6 && diffP <= diff + 1e-6);
    } else {
        return (diffP <= 1e-6 && diffP >= diff - 1e-6);
    }
}

bool ConcaveMirror::Crossing(const Mirror& other) const {
    const ConvexMirror* cv = dynamic_cast<const ConvexMirror*>(&other);
    const ConcaveMirror* cc = dynamic_cast<const ConcaveMirror*>(&other);

    if (!cv && !cc) {
        // other - прямое зеркало.
        Points c = this->CircleCenter();
        double r = this->rad_;
        auto intersections = SegmentCircle(other.getStart(), other.getEnd(), c, r);
        for (const auto& pt : intersections) {
            if (this->PointOnArc(pt, c)) return true;
        }
        return false;
    }

    // other - дуга.
    Points c1 = this->CircleCenter();
    Points c2 = other.CircleCenter();
    double r1 = this->rad_;
    double r2 = std::abs(other.getRad());

    auto intersections = CircleCircle(c1, r1, c2, r2);
    for (const auto& pt : intersections) {
        if (this->PointOnArc(pt, c1) && other.PointOnArc(pt, c2)) {
            return true;
        }
    }
    return false;
}
