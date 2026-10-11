#include "nmirror.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {
    constexpr double EPS       = 1e-9;   // "ноль" для длин/радиусов
    constexpr double POINT_EPS = 1e-6;   // допуск по расстоянию между точками
    constexpr double PAR_EPS   = 1e-10;  // синус угла для "параллельны"
    constexpr double FLAT_R    = 1e7;    // радиус, начиная с которого дуга считается прямой

    double ArcTol(double r) { return 1e-6 + 1e-12 * r; }

    bool PointsEqual(const Points& a, const Points& b) {
        return a.Dist(b) < POINT_EPS;
    }

    bool IsEndpoint(const Points& p, const Mirror& m) {
        return PointsEqual(p, m.getStart()) || PointsEqual(p, m.getEnd());
    }

    void AddUniquePoint(std::vector<Points>& pts, const Points& p) {
        for (const auto& e : pts) {
            if (PointsEqual(e, p)) return;
        }
        pts.push_back(p);
    }

    // ---------- отрезок / отрезок ----------
    void IntersectSegments(const Points& a1, const Points& a2,
                           const Points& b1, const Points& b2,
                           std::vector<Points>& result) {
        double d1x = a2.getX() - a1.getX(), d1y = a2.getY() - a1.getY();
        double d2x = b2.getX() - b1.getX(), d2y = b2.getY() - b1.getY();
        double L1 = std::sqrt(d1x * d1x + d1y * d1y);
        double L2 = std::sqrt(d2x * d2x + d2y * d2y);
        if (L1 < POINT_EPS || L2 < POINT_EPS) return;

        double wx = b1.getX() - a1.getX(), wy = b1.getY() - a1.getY();
        double cross = d1x * d2y - d1y * d2x;

        if (std::abs(cross) / (L1 * L2) < PAR_EPS) {
            // параллельны: нужна коллинеарность
            double distToLine = std::abs(wx * d1y - wy * d1x) / L1;
            if (distToLine > POINT_EPS) return;

            double L1sq = L1 * L1;
            double tb1 = (wx * d1x + wy * d1y) / L1sq;
            double tb2 = ((b2.getX() - a1.getX()) * d1x +
                          (b2.getY() - a1.getY()) * d1y) / L1sq;
            double tol = POINT_EPS / L1;

            double lo = std::max(0.0, std::min(tb1, tb2));
            double hi = std::min(1.0, std::max(tb1, tb2));
            if (lo > hi + tol) return;
            if (lo > hi) {
                lo = hi = std::min(1.0, std::max(0.0, (lo + hi) / 2));
            }

            AddUniquePoint(result, Points(a1.getX() + lo * d1x, a1.getY() + lo * d1y));
            if (hi - lo > tol) {
                AddUniquePoint(result, Points(a1.getX() + hi * d1x, a1.getY() + hi * d1y));
            }
            return;
        }

        double t = (wx * d2y - wy * d2x) / cross;
        double u = (wx * d1y - wy * d1x) / cross;
        double tolT = POINT_EPS / L1, tolU = POINT_EPS / L2;

        if (t >= -tolT && t <= 1.0 + tolT && u >= -tolU && u <= 1.0 + tolU) {
            t = std::min(1.0, std::max(0.0, t));
            AddUniquePoint(result, Points(a1.getX() + t * d1x, a1.getY() + t * d1y));
        }
    }

    // ---------- отрезок / дуга ----------
    void IntersectSegmentArc(const Points& a1, const Points& a2,
                             const Mirror& arc,
                             std::vector<Points>& result) {
        if (arc.isFlat()) {
            IntersectSegments(a1, a2, arc.getStart(), arc.getEnd(), result);
            return;
        }

        double dx = a2.getX() - a1.getX(), dy = a2.getY() - a1.getY();
        double L = std::sqrt(dx * dx + dy * dy);
        if (L < POINT_EPS) return;
        double ux = dx / L, uy = dy / L;

        Points c = arc.CircleCenter();
        double r = std::abs(arc.getRad());

        double wx = c.getX() - a1.getX(), wy = c.getY() - a1.getY();
        double proj = wx * ux + wy * uy;              // позиция основания перпендикуляра
        double perp = std::abs(wx * uy - wy * ux);    // расстояние от центра до прямой

        double tol = EPS * std::max(1.0, r);
        if (perp > r + tol) return;
        double h = (perp > r - tol) ? 0.0 : std::sqrt((r - perp) * (r + perp));

        auto check = [&](double s) {
            if (s < -POINT_EPS || s > L + POINT_EPS) return;
            Points p(a1.getX() + s * ux, a1.getY() + s * uy);
            if (arc.PointOnArc(p, c)) AddUniquePoint(result, p);
        };

        check(proj - h);
        if (h > 0) check(proj + h);
    }

    // ---------- любая пара зеркал ----------
    void IntersectMirrors(const Mirror& m1, const Mirror& m2,
                          std::vector<Points>& result) {
        if (m1.isFlat() || m2.isFlat()) {
            if (m1.isFlat() && m2.isFlat()) {
                IntersectSegments(m1.getStart(), m1.getEnd(),
                                  m2.getStart(), m2.getEnd(), result);
            } else if (m1.isFlat()) {
                IntersectSegmentArc(m1.getStart(), m1.getEnd(), m2, result);
            } else {
                IntersectSegmentArc(m2.getStart(), m2.getEnd(), m1, result);
            }
            return;
        }

        Points c1 = m1.CircleCenter();
        Points c2 = m2.CircleCenter();
        double r1 = std::abs(m1.getRad());
        double r2 = std::abs(m2.getRad());

        double dx = c2.getX() - c1.getX();
        double dy = c2.getY() - c1.getY();
        double d = std::sqrt(dx * dx + dy * dy);
        double tol = EPS * std::max(1.0, std::max(r1, r2));

        if (d < POINT_EPS) {
            // концентрические: пересечение только при равных радиусах (наложение дуг)
            if (std::abs(r1 - r2) < POINT_EPS) {
                if (m1.PointOnArc(m2.getStart(), c1)) AddUniquePoint(result, m2.getStart());
                if (m1.PointOnArc(m2.getEnd(),   c1)) AddUniquePoint(result, m2.getEnd());
                if (m2.PointOnArc(m1.getStart(), c2)) AddUniquePoint(result, m1.getStart());
                if (m2.PointOnArc(m1.getEnd(),   c2)) AddUniquePoint(result, m1.getEnd());
            }
            return;
        }

        if (d > r1 + r2 + tol) return;
        if (d < std::abs(r1 - r2) - tol) return;

        double a = (r1 * r1 - r2 * r2 + d * d) / (2.0 * d);
        double hSq = r1 * r1 - a * a;
        double h = hSq > 0 ? std::sqrt(hSq) : 0.0;

        double px = c1.getX() + a * dx / d;
        double py = c1.getY() + a * dy / d;

        Points p1(px + h * dy / d, py - h * dx / d);
        Points p2(px - h * dy / d, py + h * dx / d);

        if (m1.PointOnArc(p1, c1) && m2.PointOnArc(p1, c2)) AddUniquePoint(result, p1);
        if (m1.PointOnArc(p2, c1) && m2.PointOnArc(p2, c2)) AddUniquePoint(result, p2);
    }
} // namespace


// ============================================================
// Mirror
// ============================================================
Mirror::Mirror(Points st, Points end, double rad)
    : start_(st), end_(end), rad_(rad) {
    double d = st.Dist(end);
    if (!(d >= POINT_EPS)) {
        throw std::invalid_argument("Mirror: start and end coincide");
    }
    double r = std::abs(rad);
    if (r >= EPS && r <= FLAT_R && r < d / 2.0 - POINT_EPS) {
        throw std::invalid_argument("Mirror: |radius| is less than half of the chord");
    }
}

double Mirror::getRad() const { return rad_; }
Points Mirror::getStart() const { return start_; }
Points Mirror::getEnd() const { return end_; }
double Mirror::Dist() const { return start_.Dist(end_); }

bool Mirror::isFlat() const {
    double r = std::abs(rad_);
    return !(r >= EPS) || r > FLAT_R;   // 0, NaN, inf, огромный радиус
}

Points Mirror::CircleCenter() const {
    Points mid((start_.getX() + end_.getX()) / 2.0,
               (start_.getY() + end_.getY()) / 2.0);
    if (isFlat()) return mid;

    double r = std::abs(rad_);
    double d = Dist();
    double half = d / 2.0;
    double h = std::sqrt(std::max(0.0, (r - half) * (r + half)));  // устойчивее r^2 - half^2

    double perpX = -(end_.getY() - start_.getY()) / d;
    double perpY =  (end_.getX() - start_.getX()) / d;
    double sign = (rad_ >= 0 ? 1.0 : -1.0);

    return Points(mid.getX() + sign * h * perpX,
                  mid.getY() + sign * h * perpY);
}

bool Mirror::PointOnArc(const Points& p, const Points& center) const {
    double dx = end_.getX() - start_.getX();
    double dy = end_.getY() - start_.getY();
    double d = std::sqrt(dx * dx + dy * dy);

    if (isFlat()) {
        double cross = (p.getX() - start_.getX()) * dy - (p.getY() - start_.getY()) * dx;
        if (std::abs(cross) / d > POINT_EPS) return false;
        double t = ((p.getX() - start_.getX()) * dx + (p.getY() - start_.getY()) * dy) / (d * d);
        double tol = POINT_EPS / d;
        return t >= -tol && t <= 1.0 + tol;
    }

    double r = std::abs(rad_);
    if (std::abs(p.Dist(center) - r) > ArcTol(r)) return false;

    // Дуга - часть окружности по ту сторону хорды, куда она выгнута
    // (противоположную центру). Для полуокружности сторону задаёт знак rad_.
    double sign = (rad_ >= 0 ? 1.0 : -1.0);
    double nx =  sign * dy / d;    // = -sign * perpX
    double ny = -sign * dx / d;
    double mx = (start_.getX() + end_.getX()) / 2.0;
    double my = (start_.getY() + end_.getY()) / 2.0;

    return (p.getX() - mx) * nx + (p.getY() - my) * ny >= -POINT_EPS;
}

std::vector<Points> Mirror::Intersections(const Mirror& other) const {
    std::vector<Points> result;
    IntersectMirrors(*this, other, result);
    return result;
}

bool Mirror::Touches(const Mirror& other) const {
    return !Intersections(other).empty();
}

bool Mirror::Crossing(const Mirror& other) const {
    std::vector<Points> pts = Intersections(other);
    if (pts.empty()) return false;
    if (pts.size() >= 2) return true;

    // Одна точка: если это конец обоих зеркал - обычный стык.
    const Points& p = pts[0];
    return !(IsEndpoint(p, *this) && IsEndpoint(p, other));
}


// ============================================================
// Наследники: только задают знак радиуса
// ============================================================
ConvexMirror::ConvexMirror(Points st, Points end, double rad)
    : Mirror(st, end, std::abs(rad)) {}

ConcaveMirror::ConcaveMirror(Points st, Points end, double rad)
    : Mirror(st, end, -std::abs(rad)) {}

PlaneMirror::PlaneMirror(Points st, Points end)
    : Mirror(st, end, 0.0) {}
