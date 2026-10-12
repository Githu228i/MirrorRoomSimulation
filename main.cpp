#include "polygon.h"
#include "mirror.h"
#include "points.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>

static const char* MirrorTypeName(const Mirror& m) {
    if (m.isFlat())     return "плоское";
    if (m.getRad() > 0) return "выпуклое";
    return "вогнутое";
}

static void PrintPoint(const Points& p) {
    std::cout << "(" << p.getX() << ", " << p.getY() << ")";
}

int main() {
    std::cout << "=== Зеркальный многоугольник + трассировка луча ===\n\n";

    // ---------- 1. Количество сторон ----------
    int n = 0;
    std::cout << "Число сторон N (>= 3): ";
    if (!(std::cin >> n) || n < 3) {
        std::cerr << "Нужно целое N >= 3.\n";
        return 1;
    }

    // ---------- 2. Ввод сторон ----------
    std::cout << "\nТип стороны:\n"
              << "  0 — плоское\n"
              << "  1 — выпуклое (дополнительно вводится радиус)\n"
              << "  2 — вогнутое  (дополнительно вводится радиус)\n\n";
    std::cout << "Формат строки:\n"
              << "  x1 y1  x2 y2  type  [|radius|]\n\n";

    std::vector<Mirror> sides;
    sides.reserve(n);

    for (int i = 0; i < n; ++i) {
        double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        int t = 0;

        std::cout << "Сторона " << i
                  << " (x1 y1  x2 y2  type): ";
        if (!(std::cin >> x1 >> y1 >> x2 >> y2 >> t)) {
            std::cerr << "Ошибка чтения стороны.\n";
            return 1;
        }

        try {
            switch (t) {
                case 1: {
                    double r = 0;
                    std::cout << "  |radius| = ";
                    if (!(std::cin >> r)) { std::cerr << "bad r\n"; return 1; }
                    sides.emplace_back(Points(x1, y1), Points(x2, y2), std::abs(r));
                    break;
                }
                case 2: {
                    double r = 0;
                    std::cout << "  |radius| = ";
                    if (!(std::cin >> r)) { std::cerr << "bad r\n"; return 1; }
                    sides.emplace_back(Points(x1, y1), Points(x2, y2), -std::abs(r));
                    break;
                }
                case 0:
                default:
                    sides.emplace_back(Points(x1, y1), Points(x2, y2), 0.0);
                    break;
            }
        } catch (const std::exception& e) {
            std::cerr << "Ошибка создания зеркала " << i
                      << ": " << e.what() << "\n";
            return 2;
        }
    }

    // ---------- 3. Луч ----------
    double bx = 0, by = 0, ang = 0;
    std::cout << "\nНачало луча (x y): ";
    if (!(std::cin >> bx >> by)) { std::cerr << "bad beam pos\n"; return 1; }
    std::cout << "Угол луча (в радианах): ";
    if (!(std::cin >> ang))      { std::cerr << "bad beam angle\n"; return 1; }

    Beam beam(bx, by, ang);

    // ---------- 4. Полигон ----------
    Polygon poly(sides, beam);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "\n=== Зеркала ===\n";
    for (int i = 0; i < static_cast<int>(sides.size()); ++i) {
        const Mirror& m = sides[i];
        std::cout << "M" << i << " (" << MirrorTypeName(m) << "): ";
        PrintPoint(m.getStart());
        std::cout << " -> ";
        PrintPoint(m.getEnd());
        std::cout << "  |L|=" << m.Dist();
        if (!m.isFlat()) {
            std::cout << "  rad=" << m.getRad() << "  C=";
            PrintPoint(m.CircleCenter());
        }
        std::cout << "\n";
    }

    // ============================================================
    // 5. ПРОВЕРКА: стороны не пересекаются (кроме общих вершин соседей)
    // ============================================================
    std::cout << "\n=== Проверка многоугольника ===\n";

    // 5a. Замкнутость контура
    bool closed = true;
    for (int i = 0; i < poly.Size(); ++i) {
        const Mirror& a = sides[i];
        const Mirror& b = sides[(i + 1) % poly.Size()];
        if (a.getEnd().Dist(b.getStart()) > 1e-6) {
            std::cout << "  Разрыв контура между M" << i
                      << " и M" << ((i + 1) % poly.Size()) << ": ";
            PrintPoint(a.getEnd());
            std::cout << " != ";
            PrintPoint(b.getStart());
            std::cout << "\n";
            closed = false;
        }
    }

    // 5b. Лишние пересечения (у несоседних сторон)
bool extraCross = false;
const int N = poly.Size();
for (int i = 0; i < N; ++i) {
    for (int j = i + 1; j < N; ++j) {
        // Собираем ДОПУСТИМЫЕ точки касания для этой пары.
        std::vector<Points> allowed;
        if (j == i + 1)                allowed.push_back(sides[i].getEnd());
        if (i == 0 && j == N - 1)      allowed.push_back(sides[j].getEnd());

        std::vector<Points> pts = sides[i].Intersections(sides[j]);
        for (const Points& p : pts) {
            bool ok = false;
            for (const Points& a : allowed) {
                if (p.Dist(a) < 1e-6) { ok = true; break; }
            }
            if (ok) continue;

            std::cout << "  Лишнее пересечение M" << i
                      << " & M" << j << " в ";
            PrintPoint(p);
            std::cout << (allowed.empty() ? "  [несоседние]"
                                          : "  [соседи, не общая вершина]")
                      << "\n";
            extraCross = true;
        }
    }
}

    if (!closed) {
        std::cout << "FAIL: контур не замкнут — трассировка не запускается.\n";
        return 3;
    }
    if (extraCross || !poly.Check()) {
        std::cout << "FAIL: стороны пересекаются — трассировка не запускается.\n";
        return 4;
    }

    std::cout << "OK: стороны не пересекаются, контур замкнут.\n";

    // ---------- 6. Трассировка луча ----------
    std::cout << "\n=== Трассировка луча ===\n";
    poly.TraceBeam(100);

    std::cout << "Результат сохранён в beam.txt.\n";
    std::cout << "Список вершин/луча — в points.txt (при каждом Push()).\n";

    return 0;
}
