#ifndef FRONTENDOPERATORS_H
#define FRONTENDOPERATORS_H

#include "points.h"
#include "mirror.h"
#include "polygon.h"
#include "changepointdialog.h"

#include <vector>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

// Допустимая рабочая область и пороги (в пикселях)
namespace Limits {
constexpr double MinX = 0.0;
constexpr double MaxX = 1580.0;
constexpr double MinY = 0.0;
constexpr double MaxY = 1000.0;
constexpr double MinPointDist = 10.0;   // ближе - "слишком близко"
constexpr int    MinWalls = 4;              // сторон (углов) в замкнутой комнате: не меньше (по ТЗ)
constexpr int    MaxWalls = 9;              // и не больше
constexpr double ClickRadius = 5.0;         // клик не дальше этого от вершины или источника луча - попадание в неё
constexpr double BeamWallTolerance = 6.0;   // допуск клика по стене при выборе источника луча
constexpr double MinBeamVertexDist = 10.0;  // источник луча не ближе этого к вершине
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
constexpr double RaySpeed = 500.0;          // скорость луча в анимации (пикселей в секунду)
constexpr int    RaySpeedMin  = 100;        // минимальная скорость луча (ползунок), пикселей в секунду
constexpr int    RaySpeedMax  = 2000;       // максимальная скорость луча (ползунок)
constexpr int    RaySpeedStep = 50;         // шаг ползунка (стрелки); PageUp/PageDown = 2 шага
constexpr int    RepeatDelayMin     = 0;      // задержка между отражениями в авторежиме (мс): минимум
constexpr int    RepeatDelayMax     = 5000;   // максимум
constexpr int    RepeatDelayStep    = 100;    // шаг ползунка (0.1 с); PageUp/PageDown = 2 шага
constexpr int    RepeatDelayDefault = 500;    // значение по умолчанию
constexpr int    MaxChatMessages = 500;     // сколько сообщений хранит служебное окно
constexpr int    TraceLimitDefault = 50;    // сколько последних отрезков луча остаётся на экране (если включено удаление)
constexpr int    TraceLimitMin     = 1;     // допустимый диапазон значения в поле ввода
constexpr int    TraceLimitMax     = 1000000;
constexpr bool   EraseOldDefault   = false; // удалять ли старые отражения по умолчанию (галочка)
// Тип, в котором хранятся координаты (по возвращаемому значению Points::getX)
using CoordType = std::conditional_t<
    std::is_floating_point_v<std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Points>().getX())>>>,
    std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Points>().getX())>>,
    double>;
constexpr int    SaveDecimals      = 3;     // знаков после запятой в координатах по умолчанию (диалог сохранения)
constexpr int    SaveDecimalsMax   = std::numeric_limits<CoordType>::max_digits10;   // максимум: для double - 17
constexpr int    SaveAngleDecimals = 6;     // знаков после запятой в углах (градусы) в файле истории
constexpr int    WindowWidth  = 2000;       // размер окна (setFixedSize), как в .ui
constexpr int    WindowHeight = 1300;
constexpr double DimOpacity   = 0.35;       // непрозрачность недоступных (тусклых) элементов
constexpr int    MessageDecimals = 3;       // знаков после запятой в координатах и радиусах в сообщениях чата
constexpr int    DelayDecimals   = 1;       // знаков после запятой в подписи задержки (секунды)
constexpr double MsPerSecond     = 1000.0;  // миллисекунд в секунде
constexpr int    MinAnimMs       = 1;       // минимальная длительность анимации одного отрезка, мс
}

// Математика: число пи, обороты в радианах и градусах
namespace Math {
constexpr double Pi          = 3.14159265358979323846;
constexpr double TwoPi       = 2.0 * Pi;
constexpr double HalfTurnDeg = 180.0;
constexpr double FullTurnDeg = 360.0;
}

// Тип стены: числа совпадают с порядком в списке диалога изменения точки
// (ChangePointData::mirrorType) и с TraceSnapshot::Wall::type
namespace MirrorKind {
constexpr int Plane   = 0;   // плоская
constexpr int Concave = 1;   // вогнутая
constexpr int Convex  = 2;   // выпуклая
}

// Уровень сообщения для служебного окна
enum class NoteLevel { Info, Warning, Error, Reflection };

// Снимок комнаты и луча на момент старта симуляции (для файла с историей:
// комнату можно изменить уже после сброса пути)
struct TraceSnapshot
{
    struct Wall
    {
        double x = 0.0;        // начало стены (вершина)
        double y = 0.0;
        int    type = MirrorKind::Plane;   // см. MirrorKind
        double radius = 0.0;   // радиус кривизны (для плоской не важен)
    };

    std::vector<Wall> walls;
    int    beamWall = -1;          // индекс стены с источником (с нуля)
    double beamWallAngle = 0.0;    // угол луча к стене, градусы
};

class FrontEndOperators
{
public:
    FrontEndOperators();

    void CreatePoint(double x, double y);
    void ConnectPoints(int num);
    void ChangeMirror(int num, const ChangePointData &data);
    void DeleteLast();
    // Удаляет точку незамкнутой комнаты (num: 0 - первая, n - последняя, как в ChangeMirror):
    //  - единственная точка просто исчезает;
    //  - крайняя точка исчезает вместе со своей стеной;
    //  - у средней точки обе стены заменяются одной плоской стеной между её соседями
    //    (если она пересекает другие стены или слишком близко к ним - удаление отменяется)
    //  - в замкнутой комнате можно удалить любую вершину (стены по обе стороны от неё
    //    заменяются одной плоской), но после удаления должно остаться не меньше Limits::MinWalls
    void DeletePoint(int num);
    void Binding();

    // Пошаговая трассировка луча
    bool NextReflection();   // следующая точка отражения (false - не удалось)
    void ResetTrace();       // сбросить путь: на экране его нет сразу, а history_ очищается
        // только при первом новом отражении (traceCleared_)
    bool HasTrace() const { return !traceCleared_ && history_.size() > 1; }   // есть ли путь для показа

    // Сюда FrontEndOperators шлёт сообщения для служебного окна
    std::function<void(NoteLevel, const QString &)> notify;

    friend class MainWindow;

private:
    void Notify(NoteLevel level, const QString &text) const;
    TraceSnapshot MakeSnapshot() const;   // комната и луч на данный момент

    Polygon prototype;
    Beam tempBeam_;
    std::vector<Points> history_;   // стартовая точка + точки отражений
    std::vector<Beam> states_;      // состояния луча (точка + угол): старт + после каждого отражения
    bool looped_ = false;           // ход луча повторился - луч зациклился
    bool traceCleared_ = false;     // путь сброшен: не показывается, но history_ хранит старые данные
        // до начала новых отражений (пригодится для сохранения в файл)
    TraceSnapshot snapshot_;        // комната и луч на момент старта той симуляции, чья история в history_
};

#endif // FRONTENDOPERATORS_H
