#include "polygon.h"
#include <cmath>
#include <algorithm>
#include <iomanip>

namespace {
constexpr double EPS       = 1e-9;
constexpr double TRACE_EPS = 1e-7;   // минимальное расстояние до следующего удара
constexpr double POINT_EPS = 1e-6;

bool Close(const Points& a, const Points& b) {
    return a.Dist(b) < POINT_EPS;
}

// Пересечение луча (dir - единичный) с отрезком. Возвращает t > 0 или -1.
double RaySegmentIntersect(const Points& origin, const Points& dir,
                           const Points& a, const Points& b) {
    double dx = b.getX() - a.getX();
    double dy = b.getY() - a.getY();
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < EPS) return -1.0;

    double cross = dir.getX() * dy - dir.getY() * dx;
    if (std::abs(cross) / len < EPS) return -1.0; // параллелен

    double ox = a.getX() - origin.getX();
    double oy = a.getY() - origin.getY();

    double t = (ox * dy - oy * dx) / cross;
    double s = (ox * dir.getY() - oy * dir.getX()) / cross;
    double tolS = POINT_EPS / len;

    if (t > TRACE_EPS && s >= -tolS && s <= 1.0 + tolS) return t;
    return -1.0;
}

// Пересечение луча (dir - единичный) с дугой. Возвращает t > 0 или -1.
double RayArcIntersect(const Points& origin, const Points& dir, const Mirror& m) {
    Points c = m.CircleCenter();
    double r = std::abs(m.getRad());

    double fx = origin.getX() - c.getX();
    double fy = origin.getY() - c.getY();

    double b = fx * dir.getX() + fy * dir.getY();          // половина линейного коэф.
    double cc = fx * fx + fy * fy - r * r;
    double disc = b * b - cc;
    if (disc < 0) {
        if (disc < -EPS * std::max(1.0, r * r)) return -1.0;
        disc = 0;
    }
    double sq = std::sqrt(disc);
    double t1 = -b - sq;
    double t2 = -b + sq;

    double bestT = -1.0;
    for (double t : {t1, t2}) {
        if (t > TRACE_EPS && (bestT < 0 || t < bestT)) {
            Points p(origin.getX() + t * dir.getX(), origin.getY() + t * dir.getY());
            if (m.PointOnArc(p, c)) bestT = t;
        }
    }
    return bestT;
}

double RayMirrorIntersect(const Points& origin, const Points& dir, const Mirror& m) {
    return m.isFlat() ? RaySegmentIntersect(origin, dir, m.getStart(), m.getEnd())
                      : RayArcIntersect(origin, dir, m);
}

// Единичная нормаль, направленная против падающего луча.
Points GetNormal(const Points& hit, const Points& dir, const Mirror& m) {
    double nx, ny;
    if (m.isFlat()) {
        double dx = m.getEnd().getX() - m.getStart().getX();
        double dy = m.getEnd().getY() - m.getStart().getY();
        double len = std::sqrt(dx * dx + dy * dy);
        if (len < EPS) return Points(0, 0);
        nx = -dy / len;
        ny =  dx / len;
    } else {
        Points c = m.CircleCenter();
        double dx = hit.getX() - c.getX();
        double dy = hit.getY() - c.getY();
        double len = std::sqrt(dx * dx + dy * dy);
        if (len < EPS) return Points(0, 0);
        nx = dx / len;
        ny = dy / len;
    }
    if (nx * dir.getX() + ny * dir.getY() > 0) { nx = -nx; ny = -ny; }
    return Points(nx, ny);
}

// d' = d - 2(d·n)n, с перенормировкой от накопления ошибки
Points ReflectDirection(const Points& dir, const Points& n) {
    double dot = dir.getX() * n.getX() + dir.getY() * n.getY();
    double rx = dir.getX() - 2 * dot * n.getX();
    double ry = dir.getY() - 2 * dot * n.getY();
    double len = std::sqrt(rx * rx + ry * ry);
    if (len < EPS) return dir;
    return Points(rx / len, ry / len);
}
} // namespace


// ============================================================
// Polygon
// ============================================================
Polygon::Polygon() {}

Polygon::Polygon(const std::vector<Mirror>& sides, Beam beam)
    : beam_(beam)
{
    sides_.reserve(sides.size());
    for (const auto& s : sides) {
        if (dynamic_cast<const PlaneMirror*>(&s)) {
            sides_.push_back(std::make_unique<PlaneMirror>(s.getStart(), s.getEnd()));
        }
        else if (dynamic_cast<const ConcaveMirror*>(&s)) {
            sides_.push_back(std::make_unique<ConcaveMirror>(s.getStart(), s.getEnd(), s.getRad()));
        }
        else if (dynamic_cast<const ConvexMirror*>(&s)) {
            sides_.push_back(std::make_unique<ConvexMirror>(s.getStart(), s.getEnd(), s.getRad()));
        }
    }
}

bool Polygon::isExist() const {
    return isExist_;
}

bool Polygon::isCycled() const {
    return isCycled_;
}

void Polygon::Write() const {
    std::ofstream log("points.txt");
    if (!log.is_open()) {
        std::cerr << "The log cannot be opened." << std::endl;
        return;
    }
    log << "-------------------------------" << std::endl;
    log << "Vertices of the polygon:" << std::endl;
    for (const auto& s : sides_) {
        log << s->getStart().Cords() << std::endl;
    }
    log << "Beam: " << beam_.Cords() << std::endl;
}

void Polygon::Push(const Mirror& other) {
    if (dynamic_cast<const PlaneMirror*>(&other)) {
        sides_.push_back(std::make_unique<PlaneMirror>(
            other.getStart(), other.getEnd()
            ));
    }
    else if (dynamic_cast<const ConcaveMirror*>(&other)) {
        sides_.push_back(std::make_unique<ConcaveMirror>(
            other.getStart(), other.getEnd(), other.getRad()
            ));
    }
    else if (dynamic_cast<const ConvexMirror*>(&other)) {
        sides_.push_back(std::make_unique<ConvexMirror>(
            other.getStart(), other.getEnd(), other.getRad()
            ));
    }
    Write();
}

void Polygon::Del(int idx) {
    if (idx < 0 || idx >= Size()) return;
    if (isBeamSet_) {
        if (beamWall_ == idx)      isBeamSet_ = false;
        else if (beamWall_ > idx)  --beamWall_;
    }
    sides_.erase(sides_.begin() + idx);
    Write();
}

int Polygon::Size() const {
    return static_cast<int>(sides_.size());
}

BeamPlaceResult Polygon::PlaceBeam(const Points& click, double maxDist, double minVertexDist) {
    double best = maxDist;
    int bestIdx = -1;
    Points bestPt;

    for (int i = 0; i < Size(); ++i) {
        double d;
        Points q = sides_[i]->ClosestPoint(click, d);
        if (d <= best) {
            best = d;
            bestIdx = i;
            bestPt = q;
        }
    }
    if (bestIdx < 0) return BeamPlaceResult::NoWall;

    // Слишком близко к вершине
    for (const auto& s : sides_) {
        if (s->getStart().Dist(bestPt) <= minVertexDist ||
            s->getEnd().Dist(bestPt)   <= minVertexDist)
            return BeamPlaceResult::TooClose;
    }

    beam_ = Beam(bestPt.getX(), bestPt.getY(), beam_.getA());
    isBeamSet_ = true;
    beamWall_ = bestIdx;
    return BeamPlaceResult::Placed;
}

void Polygon::SetBeamAngle(double wallAngleDeg) {
    if (!isBeamSet_ || beamWall_ < 0 || beamWall_ >= Size()) return;

    beamWallAngle_ = wallAngleDeg;

    const Mirror& m = *sides_[beamWall_];
    const double pi = 3.14159265358979323846;

    // касательная в точке источника по ходу обхода start -> end
    double tx, ty;
    if (m.isFlat()) {
        double dx = m.getEnd().getX() - m.getStart().getX();
        double dy = m.getEnd().getY() - m.getStart().getY();
        double len = std::sqrt(dx * dx + dy * dy);
        tx = dx / len;
        ty = dy / len;
    } else {
        Points c = m.CircleCenter();
        double rx = beam_.getX() - c.getX();
        double ry = beam_.getY() - c.getY();
        double len = std::sqrt(rx * rx + ry * ry);
        rx /= len;
        ry /= len;
        double sg = (m.getRad() >= 0) ? 1.0 : -1.0;
        tx = -sg * ry;
        ty =  sg * rx;
    }

    // сторона комнаты по знаку площади контура
    double area = 0.0;
    for (const auto& s : sides_) {
        area += s->getStart().getX() * s->getEnd().getY()
                - s->getEnd().getX()   * s->getStart().getY();
    }
    double side = (area >= 0) ? 1.0 : -1.0;

    // нормаль к стене в сторону комнаты
    double nx = -side * ty;
    double ny =  side * tx;

    double th = wallAngleDeg * pi / 180.0;
    double dx = tx * std::cos(th) + nx * std::sin(th);
    double dy = ty * std::cos(th) + ny * std::sin(th);

    beam_ = Beam(beam_.getX(), beam_.getY(), std::atan2(dy, dx));
}

bool Polygon::Check() const {
    const int n = Size();
    if (n < 2) return false;

    // Вырожденный двуугольник из двух отрезков - не многоугольник
    if (n == 2 && sides_[0]->isFlat() && sides_[1]->isFlat()) return false;

    // Контур должен быть замкнутым и непрерывным
    for (int i = 0; i < n; ++i) {
        if (!Close(sides_[i]->getEnd(), sides_[(i + 1) % n]->getStart())) return false;
    }

    // Любая пара сторон: общие точки допустимы только в общих вершинах соседей
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            std::vector<Points> allowed;
            if (j == i + 1)            allowed.push_back(sides_[i]->getEnd());
            if (i == 0 && j == n - 1)  allowed.push_back(sides_[j]->getEnd());

            for (const Points& p : sides_[i]->Intersections(*sides_[j])) {
                bool ok = false;
                for (const Points& a : allowed) {
                    if (Close(p, a)) { ok = true; break; }
                }
                if (!ok) return false;
            }
        }
    }
    return true;
}

void Polygon::TraceBeam(int maxBounces) {
    std::ofstream log("beam.txt");
    if (!log.is_open()) {
        std::cerr << "beam.txt cannot be opened." << std::endl;
        return;
    }

    Points pos(beam_.getX(), beam_.getY());
    Points dir(std::cos(beam_.getA()), std::sin(beam_.getA()));

    log << std::fixed << std::setprecision(6);
    log << pos.getX() << " " << pos.getY() << std::endl;

    for (int bounce = 0; bounce < maxBounces; ++bounce) {
        double bestT = -1.0;
        int bestIdx = -1;

        for (int i = 0; i < Size(); ++i) {
            double t = RayMirrorIntersect(pos, dir, *sides_[i]);
            if (t > TRACE_EPS && (bestT < 0 || t < bestT)) {
                bestT = t;
                bestIdx = i;
            }
        }
        if (bestIdx < 0) break;   // луч вышел

        Points hit(pos.getX() + bestT * dir.getX(),
                   pos.getY() + bestT * dir.getY());
        log << hit.getX() << " " << hit.getY() << std::endl;

        Points normal = GetNormal(hit, dir, *sides_[bestIdx]);
        dir = ReflectDirection(dir, normal);
        pos = hit;
    }
}

std::pair<Beam, Points> Polygon::TraceBeamIter(const Beam& initial) const {
    Points pos(initial.getX(), initial.getY());

    Points dir(
        std::cos(initial.getA()),
        std::sin(initial.getA())
        );

    double bestT = -1.0;
    int bestIdx = -1;

    // Ищем ближайшее зеркало
    for (int i = 0; i < Size(); ++i) {
        double t = RayMirrorIntersect(pos, dir, *sides_[i]);

        if (t > TRACE_EPS && (bestT < 0 || t < bestT)) {
            bestT = t;
            bestIdx = i;
        }
    }
    // Луч никуда не попал: остаёмся на месте, а не обращаемся к sides_[-1]
    if (bestIdx < 0) return {initial, pos};
    // Точка отражения
    Points hit(
        pos.getX() + bestT * dir.getX(),
        pos.getY() + bestT * dir.getY()
        );

    // Рассчитываем отражённое направление
    Points normal = GetNormal(hit, dir, *sides_[bestIdx]);
    Points reflectedDir = ReflectDirection(dir, normal);

    double newAngle = std::atan2(
        reflectedDir.getY(),
        reflectedDir.getX()
        );

    // Создаём новый луч из точки отражения
    Beam reflectedBeam(
        hit.getX(),
        hit.getY(),
        newAngle
        );

    return {reflectedBeam, hit};
}
