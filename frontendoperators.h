#ifndef FRONTENDOPERATORS_H
#define FRONTENDOPERATORS_H

#include "points.h"
#include "mirror.h"
#include "polygon.h"
#include "changepointdialog.h"

#include <vector>
#include <string>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

// Допустимая рабочая область и пороги (в пикселях)
#include "constants.h"

// Итог загрузки комнаты из файла
enum class LoadResult
{
    Failed,         // ошибка: ничего не изменилось
    RoomOnly,       // комната загружена, луча нет (его нет в файле или он забракован)
    RoomWithBeam    // комната и луч загружены
};

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

    // Загружает замкнутую комнату (и луч, если он есть в файле и лежит на стене) из текста
    // файла в кодировке UTF-8. Формат: после строки «Углы комнаты:» идут строки с двумя
    // числами (x y) и необязательным «тип: вогнутое(R)» / «тип: выпуклое(R)»; после строки
    // «Луч:» - одна строка: x y [угол к стене в градусах]. При любой ошибке сообщение уходит
    // в notify, а текущая комната остаётся без изменений.
    LoadResult LoadRoom(const std::string &utf8Text);

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
