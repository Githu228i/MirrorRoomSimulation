#include "polygon.h"
#include <cmath>
#include <limits>
#include <algorithm>
#include <iomanip>

// ============================================================
// Вспомогательные функции для трассировки луча
// ============================================================
namespace {
    constexpr double EPS = 1e-9;

    // Является ли зеркало прямым (плоским)?
    bool IsFlat(const Mirror& m) {
        return std::abs(m.getRad()) > 1e12 || std::isinf(m.getRad());
    }

    // Пересечение луча с отрезком.
    // Возвращает параметр t > 0 (расстояние вдоль луча) или -1.
    double RaySegmentIntersect(const Points& origin, const Points& dir,
                               const Points& a, const Points& b) {
        double dx = b.getX() - a.getX();
        double dy = b.getY() - a.getY();

        double cross = dir.getX() * dy - dir.getY() * dx;
        if (std::abs(cross) < EPS) return -1.0; // Луч параллелен отрезку

        double ox = a.getX() - origin.getX();
        double oy = a.getY() - origin.getY();

        double t = (ox * dy - oy * dx) / cross;
        double s = (ox * dir.getY() - oy * dir.getX()) / cross;

        if (t > EPS && s >= -EPS && s <= 1.0 + EPS) {
            return t;
        }
        return -1.0;
    }

    // Пересечение луча с дугой (кривым зеркалом).
    // Возвращает параметр t > 0 или -1.
    double RayArcIntersect(const Points& origin, const Points& dir, const Mirror& m) {
        Points c = m.CircleCenter();
        double r = std::abs(m.getRad());

        double fx = origin.getX() - c.getX();
        double fy = origin.getY() - c.getY();

        // Квадратное уравнение: a*t^2 + b*t + c = 0
        double a = dir.getX() * dir.getX() + dir.getY() * dir.getY();
        double b = 2.0 * (fx * dir.getX() + fy * dir.getY());
        double cc = fx * fx + fy * fy - r * r;

        double discriminant = b * b - 4 * a * cc;
        if (discriminant < -EPS) return -1.0;
        if (discriminant < 0) discriminant = 0;

        double sqrtD = std::sqrt(discriminant);
        double t1 = (-b - sqrtD) / (2 * a);
        double t2 = (-b + sqrtD) / (2 * a);

        double bestT = -1.0;

        // Проверяем оба корня, выбираем ближайший, лежащий на дуге
        if (t1 > EPS) {
            Points p(origin.getX() + t1 * dir.getX(),
                     origin.getY() + t1 * dir.getY());
            if (m.PointOnArc(p, c)) bestT = t1;
        }
        if (t2 > EPS && (bestT < 0 || t2 < bestT)) {
            Points p(origin.getX() + t2 * dir.getX(),
                     origin.getY() + t2 * dir.getY());
            if (m.PointOnArc(p, c)) bestT = t2;
        }

        return bestT;
    }

    // Универсальное пересечение луча с зеркалом
    double RayMirrorIntersect(const Points& origin, const Points& dir, const Mirror& m) {
        if (IsFlat(m)) {
            return RaySegmentIntersect(origin, dir, m.getStart(), m.getEnd());
        } else {
            return RayArcIntersect(origin, dir, m);
        }
    }

    // Нормаль к зеркалу в точке пересечения.
    // Автоматически разворачивается против направления падения.
    Points GetNormal(const Points& hit, const Points& dir, const Mirror& m) {
        Points normal;

        if (IsFlat(m)) {
            Points a = m.getStart();
            Points b = m.getEnd();
            double dx = b.getX() - a.getX();
            double dy = b.getY() - a.getY();
            double len = std::sqrt(dx * dx + dy * dy);
            if (len < EPS) return Points(0, 0);

            // Перпендикуляр к отрезку
            normal = Points(-dy / len, dx / len);
        } else {
            Points c = m.CircleCenter();
            double dx = hit.getX() - c.getX();
            double dy = hit.getY() - c.getY();
            double len = std::sqrt(dx * dx + dy * dy);
            if (len < EPS) return Points(0, 0);

            // Радиальная нормаль дуги
            normal = Points(dx / len, dy / len);
        }

        // Убеждаемся, что нормаль направлена против луча
        double dot = normal.getX() * dir.getX() + normal.getY() * dir.getY();
        if (dot > 0) {
            normal = Points(-normal.getX(), -normal.getY());
        }
        return normal;
    }

    // Отражение вектора направления относительно нормали:  d' = d - 2(d·n)n
    Points ReflectDirection(const Points& dir, const Points& normal) {
        double dot = dir.getX() * normal.getX() + dir.getY() * normal.getY();
        return Points(dir.getX() - 2 * dot * normal.getX(),
                      dir.getY() - 2 * dot * normal.getY());
    }
} // anonymous namespace


// ============================================================
// Существующие методы Polygon (без изменений)
// ============================================================
Polygon::Polygon(const std::vector<Mirror>& sides, Beam beam) {
    sides_ = sides;
    beam_ = beam;
}

void Polygon::Write() {
    std::ofstream log("points.txt");
    if (log.is_open()) {
        log << std::endl;
        log << "-------------------------------" << std::endl;
        log << "Vertices of the polygon:" << std::endl;
        for (int i = 0; i < Size(); ++i) {
            log << sides_[i].getStart().Cords();
        }
        log << "Beam: " << beam_.Cords();
    } else {
        std::cerr << "The log cannot be opened." << std::endl;
        return;
    }
    log.close();
}

void Polygon::Push(const Mirror& other) {
    sides_.push_back(other);
    Write();
}

void Polygon::Del(int idx) {
    std::vector<Mirror> new_sides;
    for (int i = 0; i < Size(); ++i) {
        if (i == idx) continue;
        new_sides.push_back(sides_[i]);
    }
    sides_ = new_sides;
    Write();
}

int Polygon::Size() {
    return sides_.size();
}

bool Polygon::Check() {
    for (int i = 0; i < Size(); ++i) {
        for (int j = 0; j < Size(); ++j) {
            if (i == j) continue;
            if (sides_[i].Crossing(sides_[j])) return false;
        }
    }
    return true;
}


// ============================================================
// НОВЫЙ МЕТОД: трассировка луча
// ============================================================
void Polygon::TraceBeam(int maxBounces) {
    std::ofstream log("beam.txt");
    if (!log.is_open()) {
        std::cerr << "beam.txt cannot be opened." << std::endl;
        return;
    }

    // Стартовая позиция и направление луча
    Points pos(beam_.getX(), beam_.getY());
    Points dir(std::cos(beam_.getA()), std::sin(beam_.getA()));

    log << std::fixed << std::setprecision(6);
    log << pos.getX() << " " << pos.getY() << std::endl;

    for (int bounce = 0; bounce < maxBounces; ++bounce) {
        double bestT = -1.0;
        int bestIdx = -1;

        // Ищем ближайшее пересечение среди всех зеркал
        for (int i = 0; i < Size(); ++i) {
            double t = RayMirrorIntersect(pos, dir, sides_[i]);
            if (t > EPS && (bestT < 0 || t < bestT)) {
                bestT = t;
                bestIdx = i;
            }
        }

        // Если пересечений нет — луч покинул комнату
        if (bestIdx < 0) {
            break;
        }

        // Точка удара
        Points hit(pos.getX() + bestT * dir.getX(),
                   pos.getY() + bestT * dir.getY());
        log << hit.getX() << " " << hit.getY() << std::endl;

        // Отражение
        Points normal = GetNormal(hit, dir, sides_[bestIdx]);
        dir = ReflectDirection(dir, normal);
        pos = hit;
    }

    log.close();
}
