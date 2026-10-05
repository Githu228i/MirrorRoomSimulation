#include "nmirror.h"
#include "polygon.h"
#include <iostream>
#include <vector>
#include <limits>
#include <iomanip>

using namespace std;


// ============================================================
// Вспомогательные функции ввода
// ============================================================

double readDouble(const string& prompt) {
    double val;
    while (true) {
        cout << prompt;
        if (cin >> val) return val;
        cout << "  [!] Неверный ввод, повторите." << endl;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

int readInt(const string& prompt, int minVal = 1) {
    int val;
    while (true) {
        cout << prompt;
        if ((cin >> val) && val >= minVal) return val;
        cout << "  [!] Ожидается целое число >= " << minVal << endl;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

// ============================================================
// ТЕСТ 1: Многоугольник только из прямых зеркал
// ============================================================
void testFlatPolygon() {
    cout << "\n======================================" << endl;
    cout << " ТЕСТ 1: Многоугольник из прямых зеркал" << endl;
    cout << "======================================" << endl;

    int n = readInt("Введите количество сторон (>= 3): ", 3);

    vector<Mirror> sides;
    sides.reserve(n);

    for (int i = 0; i < n; ++i) {
        cout << "\n--- Сторона " << (i + 1) << " ---" << endl;
        double x1 = readDouble("  x1: ");
        double y1 = readDouble("  y1: ");
        double x2 = readDouble("  x2: ");
        double y2 = readDouble("  y2: ");
        // Прямое зеркало: rad = 0
        sides.emplace_back(Points(x1, y1), Points(x2, y2), 0.0);
    }

    Beam dummyBeam(0, 0, 0);
    Polygon poly(sides, dummyBeam);

    cout << "\n[Результат] ";
    if (poly.Check()) {
        cout << "✅ Стороны НЕ пересекаются (валидный многоугольник)." << endl;
    } else {
        cout << "❌ Обнаружены пересечения между сторонами!" << endl;
    }

    // Детальный отчёт: какие именно стороны пересекаются
    cout << "\nДетали:" << endl;
    bool any = false;
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (sides[i].Crossing(sides[j])) {
                cout << "  ⚠ Сторона " << (i + 1) << " пересекается со стороной " << (j + 1) << endl;
                any = true;
            }
        }
    }
    if (!any) cout << "  Пар с пересечениями не найдено." << endl;
}

// ============================================================
// ТЕСТ 2: Многоугольник из выпуклых/вогнутых зеркал
// ============================================================
void testCurvedPolygon() {
    cout << "\n======================================" << endl;
    cout << " ТЕСТ 2: Многоугольник из кривых зеркал" << endl;
    cout << "======================================" << endl;
    cout << "Условные обозначения радиуса:" << endl;
    cout << "  rad > 0  — выпуклое зеркало" << endl;
    cout << "  rad < 0  — вогнутое зеркало" << endl;
    cout << "  |rad| должен быть >= половины длины хорды" << endl;

    int n = readInt("Введите количество сторон (>= 3): ", 3);

    vector<Mirror> sides;
    sides.reserve(n);

    for (int i = 0; i < n; ++i) {
        cout << "\n--- Сторона " << (i + 1) << " ---" << endl;
        double x1  = readDouble("  x1: ");
        double y1  = readDouble("  y1: ");
        double x2  = readDouble("  x2: ");
        double y2  = readDouble("  y2: ");
        double rad = readDouble("  Радиус кривизны (+/-): ");

        if (rad == 0.0) {
            cout << "  [i] rad=0, буду трактовать как прямое зеркало." << endl;
        }
        sides.emplace_back(Points(x1, y1), Points(x2, y2), rad);
    }

    Beam dummyBeam(0, 0, 0);
    Polygon poly(sides, dummyBeam);

    cout << "\n[Результат] ";
    if (poly.Check()) {
        cout << "✅ Стороны НЕ пересекаются." << endl;
    } else {
        cout << "❌ Обнаружены пересечения между сторонами!" << endl;
    }

    cout << "\nДетали:" << endl;
    bool any = false;
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (sides[i].Crossing(sides[j])) {
                cout << "  ⚠ Сторона " << (i + 1) << " пересекается со стороной " << (j + 1) << endl;
                any = true;
            }
        }
    }
    if (!any) cout << "  Пар с пересечениями не найдено." << endl;
}

// ============================================================
// ТЕСТ 3: Смешанный многоугольник (прямые + кривые)
// ============================================================
void testMixedPolygon() {
    cout << "\n======================================" << endl;
    cout << " ТЕСТ 3: Смешанный многоугольник" << endl;
    cout << "======================================" << endl;
    cout << "Типы сторон:" << endl;
    cout << "  1 — прямое зеркало" << endl;
    cout << "  2 — выпуклое зеркало  (rad > 0)" << endl;
    cout << "  3 — вогнутое зеркало  (rad < 0)" << endl;

    int n = readInt("Введите количество сторон (>= 3): ", 3);

    vector<Mirror> sides;
    sides.reserve(n);

    for (int i = 0; i < n; ++i) {
        cout << "\n--- Сторона " << (i + 1) << " ---" << endl;
        int type = readInt("  Тип (1/2/3): ", 1);
        while (type < 1 || type > 3) {
            cout << "  [!] Допустимы значения 1, 2 или 3. Повторите." << endl;
            type = readInt("  Тип (1/2/3): ", 1);
        }

        double x1 = readDouble("  x1: ");
        double y1 = readDouble("  y1: ");
        double x2 = readDouble("  x2: ");
        double y2 = readDouble("  y2: ");

        double rad = 0.0;
        if (type == 2 || type == 3) {
            rad = readDouble("  Радиус кривизны (>0): ");
            if (rad <= 0) {
                cout << "  [!] Радиус должен быть > 0, беру модуль." << endl;
                rad = std::abs(rad);
            }
            if (type == 3) rad = -rad;  // вогнутое → отрицательный rad
        }

        sides.emplace_back(Points(x1, y1), Points(x2, y2), rad);
    }

    Beam dummyBeam(0, 0, 0);
    Polygon poly(sides, dummyBeam);

    cout << "\n[Результат] ";
    if (poly.Check()) {
        cout << "✅ Стороны НЕ пересекаются." << endl;
    } else {
        cout << "❌ Обнаружены пересечения между сторонами!" << endl;
    }

    cout << "\nДетали:" << endl;
    bool any = false;
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (sides[i].Crossing(sides[j])) {
                cout << "  ⚠ Сторона " << (i + 1) << " пересекается со стороной " << (j + 1) << endl;
                any = true;
            }
        }
    }
    if (!any) cout << "  Пар с пересечениями не найдено." << endl;
}

// ============================================================
// Главное меню
// ============================================================
int main() {
    cout << fixed << setprecision(3);
    cout << "===== Интерактивные тесты пересечений многоугольников =====" << endl;

    while (true) {
        cout << "\nВыберите тест:" << endl;
        cout << "  1 — Многоугольник из прямых зеркал" << endl;
        cout << "  2 — Многоугольник из кривых зеркал (выпуклые/вогнутые)" << endl;
        cout << "  3 — Смешанный многоугольник" << endl;
        cout << "  0 — Выход" << endl;

        int choice = readInt("Ваш выбор: ", 0);
        if (choice == 0) {
            cout << "Завершение работы. Пока!" << endl;
            break;
        } else if (choice == 1) {
            testFlatPolygon();
        } else if (choice == 2) {
            testCurvedPolygon();
        } else if (choice == 3) {
            testMixedPolygon();
        } else {
            cout << "[!] Нет такого пункта меню." << endl;
        }
    }
    return 0;
}
