#include "mirror.h"

#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>


// ============================================================
// Вспомогательные функции пересечений
// ============================================================
namespace {
constexpr double EPS       = 1e-9;
constexpr double POINT_EPS = 1e-6;
constexpr double PAR_EPS   = 1e-10;
constexpr double FLAT_R    = 1e7;

double ArcTol(double r) { return 1e-6 + 1e-12 * r; }

bool PointsEqual(const Points& a, const Points& b) {
    return a.Dist(b) < POINT_EPS;
}
bool IsEndpoint(const Points& p, const Mirror& m) {
    return PointsEqual(p, m.getStart()) || PointsEqual(p, m.getEnd());
}
void AddUniquePoint(std::vector<Points>& pts, const Points& p) {
    for (const auto& e : pts) if (PointsEqual(e, p)) return;
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
        if (lo > hi) lo = hi = std::min(1.0, std::max(0.0, (lo + hi) / 2));

        AddUniquePoint(result, Points(a1.getX() + lo * d1x, a1.getY() + lo * d1y));
        if (hi - lo > tol)
            AddUniquePoint(result, Points(a1.getX() + hi * d1x, a1.getY() + hi * d1y));
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
    double proj = wx * ux + wy * uy;
    double perp = std::abs(wx * uy - wy * ux);

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

// ---------- дуга / дуга ----------
void IntersectArcs(const Mirror& m1, const Mirror& m2,
                   std::vector<Points>& result) {
    Points c1 = m1.CircleCenter();
    Points c2 = m2.CircleCenter();
    double r1 = std::abs(m1.getRad());
    double r2 = std::abs(m2.getRad());

    double dx = c2.getX() - c1.getX();
    double dy = c2.getY() - c1.getY();
    double d = std::sqrt(dx * dx + dy * dy);
    double tol = EPS * std::max(1.0, std::max(r1, r2));

    if (d < POINT_EPS) {
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
    : start_(st), end_(end), rad_(rad)
{
    double d = st.Dist(end);
    if (!(d >= POINT_EPS))
        throw std::invalid_argument("Mirror: start and end coincide");
    double r = std::abs(rad);
    if (r >= EPS && r <= FLAT_R && r < d / 2.0 - POINT_EPS)
        throw std::invalid_argument("Mirror: |radius| is less than half of the chord");
}

Mirror::Mirror(Points st, Points end)
    : Mirror(st, end, 0.0)
{
}

double Mirror::getRad()    const { return rad_; }
double Mirror::getRadius() const { return std::abs(rad_); }
Points Mirror::getStart()  const { return start_; }
Points Mirror::getEnd()    const { return end_; }
double Mirror::Dist()      const { return start_.Dist(end_); }

bool Mirror::isFlat() const
{
    double r = std::abs(rad_);
    return !(r >= EPS) || r > FLAT_R;
}

/*
 * Центр окружности со стороной, заданной знаком rad:
 * rad > 0 -> слева от start -> end, rad < 0 -> справа.
 */
Points Mirror::CircleCenter() const
{
    return CircleCenter(rad_ >= 0);
}

/*
 * Центр окружности с явно заданной стороной.
 *
 * leftSide = true  -> центр слева от направления start -> end
 * leftSide = false -> справа.
 *
 * Для отрезка возвращается середина хорды.
 */
Points Mirror::CircleCenter(bool leftSide) const
{
    Points mid((start_.getX() + end_.getX()) / 2.0,
               (start_.getY() + end_.getY()) / 2.0);
    if (isFlat()) return mid;

    double r = std::abs(rad_);
    double d = Dist();
    double half = d / 2.0;
    double h = std::sqrt(std::max(0.0, (r - half) * (r + half)));

    // Левая нормаль к start -> end: (-dy, dx)
    double perpX = -(end_.getY() - start_.getY()) / d;
    double perpY =  (end_.getX() - start_.getX()) / d;
    double sign = leftSide ? 1.0 : -1.0;

    return Points(mid.getX() + sign * h * perpX,
                  mid.getY() + sign * h * perpY);
}

bool Mirror::flatPointOnArc(const Points& p) const
{
    double dx = end_.getX() - start_.getX();
    double dy = end_.getY() - start_.getY();
    double d = std::sqrt(dx * dx + dy * dy);
    double cross = (p.getX() - start_.getX()) * dy - (p.getY() - start_.getY()) * dx;
    if (std::abs(cross) / d > POINT_EPS) return false;
    double t = ((p.getX() - start_.getX()) * dx +
                (p.getY() - start_.getY()) * dy) / (d * d);
    double tol = POINT_EPS / d;
    return t >= -tol && t <= 1.0 + tol;
}

bool Mirror::arcPointOnArc(const Points& p, const Points& center) const
{
    double r = std::abs(rad_);
    if (std::abs(p.Dist(center) - r) > ArcTol(r)) return false;

    double dx = end_.getX() - start_.getX();
    double dy = end_.getY() - start_.getY();
    double d = std::sqrt(dx * dx + dy * dy);

    double sign = (rad_ >= 0 ? 1.0 : -1.0);
    double nx =  sign * dy / d;
    double ny = -sign * dx / d;
    double mx = (start_.getX() + end_.getX()) / 2.0;
    double my = (start_.getY() + end_.getY()) / 2.0;

    return (p.getX() - mx) * nx + (p.getY() - my) * ny >= -POINT_EPS;
}

bool Mirror::PointOnArc(const Points& p, const Points& center) const
{
    if (isFlat()) return flatPointOnArc(p);
    return arcPointOnArc(p, center);
}

Points Mirror::ClosestPoint(const Points& p, double& dist) const
{
    if (isFlat()) {
        double dx = end_.getX() - start_.getX();
        double dy = end_.getY() - start_.getY();
        double len2 = dx * dx + dy * dy;
        double t = ((p.getX() - start_.getX()) * dx +
                    (p.getY() - start_.getY()) * dy) / len2;
        t = std::clamp(t, 0.0, 1.0);
        Points q(start_.getX() + t * dx, start_.getY() + t * dy);
        dist = p.Dist(q);
        return q;
    }

    Points c = CircleCenter();
    double r = getRadius();
    double vx = p.getX() - c.getX();
    double vy = p.getY() - c.getY();
    double len = std::sqrt(vx * vx + vy * vy);

    if (len > EPS) {
        Points q(c.getX() + vx / len * r, c.getY() + vy / len * r);
        if (PointOnArc(q, c)) {
            dist = std::abs(len - r);
            return q;
        }
    }

    double ds = p.Dist(start_);
    double de = p.Dist(end_);
    if (ds <= de) { dist = ds; return start_; }
    dist = de;
    return end_;
}

std::vector<Points> Mirror::Intersections(const Mirror& other) const
{
    std::vector<Points> result;
    bool f1 = isFlat(), f2 = other.isFlat();

    if (f1 && f2) {
        IntersectSegments(start_, end_, other.getStart(), other.getEnd(), result);
    } else if (f1) {
        IntersectSegmentArc(start_, end_, other, result);
    } else if (f2) {
        IntersectSegmentArc(other.getStart(), other.getEnd(), *this, result);
    } else {
        IntersectArcs(*this, other, result);
    }
    return result;
}

bool Mirror::Touches(const Mirror& other) const
{
    return !Intersections(other).empty();
}

bool Mirror::Crossing(const Mirror& other) const
{
    std::vector<Points> pts = Intersections(other);
    if (pts.empty()) return false;
    if (pts.size() >= 2) return true;

    const Points& p = pts[0];
    return !(IsEndpoint(p, *this) && IsEndpoint(p, other));
}


// ============================================================
// Наследники: конструкторы
// ============================================================
NotPlaneMirror::NotPlaneMirror(
    Points st,
    Points end,
    double rad
    )
    : Mirror(st, end, rad)
{
}

ConvexMirror::ConvexMirror(
    Points st,
    Points end,
    double rad
    )
    : NotPlaneMirror(st, end, std::abs(rad))
{
}

ConcaveMirror::ConcaveMirror(
    Points st,
    Points end,
    double rad
    )
    : NotPlaneMirror(st, end, -std::abs(rad))
{
}

PlaneMirror::PlaneMirror(
    Points st,
    Points end
    )
    : Mirror(st, end, 0.0)
{
}

bool PlaneMirror::isFlat() const
{
    return true;
}


// ============================================================
// Рисование
// ============================================================

/*
 * Рисование круглой дуги.
 *
 * centerLeft:
 *   true  -> центр окружности слева от start -> end
 *   false -> справа.
 */
static void drawCircularArc(
    QPainter& painter,
    const Mirror& mirror,
    bool centerLeft
    )
{
    const double x1 = mirror.getStart().getX();
    const double y1 = mirror.getStart().getY();
    const double x2 = mirror.getEnd().getX();
    const double y2 = mirror.getEnd().getY();

    // Отрезок (rad == 0 или огромный радиус)
    if (mirror.isFlat())
    {
        painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
        return;
    }

    const double pi = 3.14159265358979323846;

    const Points center = mirror.CircleCenter(centerLeft);
    const double cx = center.getX();
    const double cy = center.getY();
    const double radius = mirror.getRadius();

    const double startAngle = std::atan2(y1 - cy, x1 - cx);
    const double endAngle   = std::atan2(y2 - cy, x2 - cx);

    /*
     * Величина МЕНЬШЕЙ дуги, нормализованная в [-pi; pi].
     * Знак определяется стороной центра: при центре слева
     * обход start -> end идёт в положительном направлении
     * atan2, при центре справа - в отрицательном.
     */
    double delta = endAngle - startAngle;

    while (delta > pi)
        delta -= 2.0 * pi;

    while (delta < -pi)
        delta += 2.0 * pi;

    delta = std::abs(delta) * (centerLeft ? 1.0 : -1.0);

    /*
     * Дуга рисуется ломаной: точки считаются в double, а концы ставятся
     * ровно в вершины. delta уже содержит знак обхода (в тех же углах atan2,
     * что и startAngle), поэтому пересчитывать направление под Qt не нужно.
     */
    const double arcLength = std::abs(delta) * radius;
    int segments = static_cast<int>(std::ceil(arcLength / MirrorDraw::SegmentLength));
    segments = std::max(segments, MirrorDraw::MinSegments);
    segments = std::min(segments, MirrorDraw::MaxSegments);

    QPolygonF poly;
    poly.reserve(segments + 1);
    for (int k = 0; k <= segments; ++k)
    {
        const double a = startAngle + delta * static_cast<double>(k) / segments;
        poly << QPointF(cx + radius * std::cos(a), cy + radius * std::sin(a));
    }
    poly.first() = QPointF(x1, y1);
    poly.last()  = QPointF(x2, y2);

    painter.drawPolyline(poly);
}


/*
 * Обычное плоское зеркало.
 */
void PlaneMirror::Draw(
    QPainter& painter,
    bool insideOnLeft
    ) const
{
    Q_UNUSED(insideOnLeft);

    painter.drawLine(
        QPointF(start_.getX(), start_.getY()),
        QPointF(end_.getX(), end_.getY())
        );
}


/*
 * ВЫПУКЛОЕ зеркало: центр окружности внутри полигона.
 */
void ConvexMirror::Draw(
    QPainter& painter,
    bool insideOnLeft
    ) const
{
    drawCircularArc(painter, *this, insideOnLeft);
}


/*
 * ВОГНУТОЕ зеркало: центр окружности снаружи полигона,
 * дуга выгибается внутрь комнаты.
 */
void ConcaveMirror::Draw(
    QPainter& painter,
    bool insideOnLeft
    ) const
{
    drawCircularArc(painter, *this, !insideOnLeft);
}
