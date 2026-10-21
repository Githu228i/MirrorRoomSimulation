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
constexpr int    TraceLimitMax     = 1000;
constexpr bool   EraseOldDefault   = false; // удалять ли старые отражения по умолчанию (галочка)
constexpr int    SaveDecimals      = 3;     // знаков после запятой в координатах при сохранении истории в файл
}

// Уровень сообщения для служебного окна
enum class NoteLevel { Info, Warning, Error, Reflection };

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
    void ResetTrace();       // сбросить путь: на экране его нет сразу, а history_ очищается
        // только при первом новом отражении (traceCleared_)
    bool HasTrace() const { return !traceCleared_ && history_.size() > 1; }   // есть ли путь для показа

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
    bool traceCleared_ = false;     // путь сброшен: не показывается, но history_ хранит старые данные
        // до начала новых отражений (пригодится для сохранения в файл)
};

#endif // FRONTENDOPERATORS_H
