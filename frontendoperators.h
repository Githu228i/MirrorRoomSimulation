#ifndef FRONTENDOPERATORS_H
#define FRONTENDOPERATORS_H

#include "points.h"
#include "mirror.h"
#include "polygon.h"
#include "changepointdialog.h"

#include <vector>
#include <functional>

// Допустимая рабочая область и пороги (в пикселях)
namespace Limits {
constexpr double MinX = 0.0;
constexpr double MaxX = 1580.0;
constexpr double MinY = 0.0;
constexpr double MaxY = 1000.0;
constexpr double MinPointDist = 10.0;   // ближе - "слишком близко"
}

// Оформление: размеры при рисовании (в пикселях).
// WallWidth учитывается и при проверке пересечений: линии стен
// не должны сливаться, то есть расстояние между ними не меньше толщины.
namespace Style {
constexpr int    WallWidth   = 5;     // толщина линий стен
constexpr double PointRadius = 4.0;   // радиус вершин полигона
constexpr double BeamRadius  = 4.0;   // радиус точки источника луча
constexpr int    TraceWidth  = 2;     // толщина линий отражений
constexpr int    ReflectPointWidth = WallWidth + 1;   // оранжевая точка отражения: на пиксель толще линии комнаты
constexpr int    ReflectionAnimMs = 1000;   // длительность анимации одного отражения (мс)
}

// Уровень сообщения для служебного окна
enum class NoteLevel { Info, Warning, Error };

class FrontEndOperators
{
public:
    FrontEndOperators();

    void CreatePoint(double x, double y);
    void ConnectPoints(int num);
    void ChangeMirror(int num, const ChangePointData &data);
    void DeleteLast();
    void Binding();

    // Пошаговая трассировка луча
    bool NextReflection();   // следующая точка отражения (false - не удалось)
    void ResetTrace();       // начать путь заново (история = только старт)

    // Сюда FrontEndOperators шлёт сообщения для служебного окна
    std::function<void(NoteLevel, const QString &)> notify;

    friend class MainWindow;

private:
    void Notify(NoteLevel level, const QString &text) const;

    Polygon prototype;
    Beam tempBeam_;
    std::vector<Points> history_;   // стартовая точка + точки отражений
    std::vector<Beam> states_;      // состояния луча (точка + угол): старт + после каждого отражения
    bool looped_ = false;           // ход луча повторился - луч зациклился
};

#endif // FRONTENDOPERATORS_H
