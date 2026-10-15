#include "frontendoperators.h"
#include <QWidget>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cmath>

namespace {

// Тип зеркала: 0 - плоское, 1 - вогнутое, 2 - выпуклое
int TypeOf(const Mirror &m)
{
    if (dynamic_cast<const ConcaveMirror *>(&m)) return 1;
    if (dynamic_cast<const ConvexMirror *>(&m))  return 2;
    return 0;
}

// Создаёт зеркало нужного типа. Если радиус меньше половины длины зеркала,
// кривое зеркало невозможно, поэтому вместо него делается плоское
// (*flattened = true), а исключение не бросается.
std::unique_ptr<Mirror> MakeMirror(int type,
                                   const Points &a,
                                   const Points &b,
                                   double radius,
                                   bool *flattened)
{
    if (flattened) *flattened = false;

    if (type != 0 && !(radius >= a.Dist(b) / 2.0)) {
        if (flattened) *flattened = true;
        type = 0;
    }

    switch (type) {
    case 1:  return std::make_unique<ConcaveMirror>(a, b, radius);
    case 2:  return std::make_unique<ConvexMirror>(a, b, radius);
    default: return std::make_unique<PlaneMirror>(a, b);
    }
}

// Одинаковое состояние луча: та же точка и то же направление
bool SameState(const Beam &a, const Beam &b)
{
    const double pi = 3.14159265358979323846;
    const double posEps = 1e-3;     // пиксели
    const double angEps = 1e-5;     // радианы

    if (std::abs(a.getX() - b.getX()) > posEps ||
        std::abs(a.getY() - b.getY()) > posEps)
        return false;

    return std::abs(std::remainder(a.getA() - b.getA(), 2.0 * pi)) <= angEps;
}

QString Fmt(double v)
{
    return QString::number(v, 'f', 1);
}

QString PointStr(const Points &p)
{
    return QString("(%1; %2)").arg(Fmt(p.getX()), Fmt(p.getY()));
}

QString TypeName(const Mirror &m)
{
    switch (TypeOf(m)) {
    case 1:  return QString("вогнутое, R = %1").arg(Fmt(m.getRadius()));
    case 2:  return QString("выпуклое, R = %1").arg(Fmt(m.getRadius()));
    default: return QString("плоское");
    }
}

// Проверка набора стен: стены не должны пересекаться и касаться друг друга
// нигде, кроме общей вершины соседних стен (как Polygon::Check, но работает
// и для разомкнутой цепочки). only - индексы изменённых стен: проверяются
// только пары с их участием (пусто - все пары). При ошибке в *a и *b
// возвращаются индексы пересекающихся стен (a < b).
bool WallsValid(const std::vector<const Mirror *> &w,
                bool closed,
                const std::vector<int> &only,
                int *a,
                int *b)
{
    const int n = static_cast<int>(w.size());

    auto involved = [&](int i) {
        if (only.empty()) return true;
        return std::find(only.begin(), only.end(), i) != only.end();
    };

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (!involved(i) && !involved(j)) continue;

            std::vector<Points> allowed;
            if (j == i + 1)                     allowed.push_back(w[i]->getEnd());
            if (closed && i == 0 && j == n - 1) allowed.push_back(w[j]->getEnd());

            for (const Points &p : w[i]->Intersections(*w[j])) {
                bool ok = false;
                for (const Points &q : allowed) {
                    if (p.Dist(q) < 1e-6) { ok = true; break; }
                }
                if (!ok) {
                    if (a) *a = i;
                    if (b) *b = j;
                    return false;
                }
            }
        }
    }
    return true;
}

} // namespace

FrontEndOperators::FrontEndOperators() {}

void FrontEndOperators::Notify(NoteLevel level, const QString &text) const
{
    if (notify) notify(level, text);
}

void FrontEndOperators::CreatePoint(double x, double y) {
    if (prototype.sides_.size() >= 8) {
        Notify(NoteLevel::Warning,
               "Достигнут предел: в комнате не больше 9 сторон. "
               "Новую точку поставить нельзя, замкните комнату.");
        return;
    }

    if (!prototype.isExist()) {
        prototype.isExist_ = true;
        prototype.first = Points(x, y);
        Notify(NoteLevel::Info,
               QString("Первая точка поставлена: %1.").arg(PointStr(Points(x, y))));
        return;
    }

    const Points from = prototype.sides_.empty()
                            ? prototype.first
                            : prototype.sides_.back()->getEnd();
    const Points to(x, y);

    try {
        // Новая стена не должна пересекать уже поставленные
        PlaneMirror wall(from, to);

        std::vector<const Mirror *> w;
        for (const auto &side : prototype.sides_)
            w.push_back(side.get());
        w.push_back(&wall);

        int a = 0, b = 0;
        if (!WallsValid(w, false, {static_cast<int>(w.size()) - 1}, &a, &b)) {
            Notify(NoteLevel::Error,
                   QString("Нельзя поставить точку: новая стена пересекает или касается стены %1.")
                       .arg(a + 1));
            return;
        }

        prototype.sides_.push_back(std::make_unique<PlaneMirror>(from, to));
    } catch (const std::exception &e) {
        Notify(NoteLevel::Error,
               QString("Не удалось поставить точку: %1").arg(QString::fromUtf8(e.what())));
        return;
    }

    Notify(NoteLevel::Info,
           QString("Точка %1 поставлена: %2.")
               .arg(static_cast<int>(prototype.sides_.size()) + 1)
               .arg(PointStr(to)));
}

void FrontEndOperators::Binding() {
    ConnectPoints(0);
}

void FrontEndOperators::ConnectPoints(int num)
{
    if (num < 0 || num >= static_cast<int>(prototype.sides_.size()))
        return;

    Points start = prototype.sides_[num]->getStart();
    Points end = prototype.sides_.back()->getEnd();

    // Замыкающая стена нулевой длины невозможна
    if (start.Dist(end) < 1e-6) {
        Notify(NoteLevel::Error,
               "Нельзя замкнуть комнату: начальная и конечная точки совпадают.");
        return;
    }

    // Замыкающая стена не должна пересекать остальные
    {
        PlaneMirror closing(end, start);

        std::vector<const Mirror *> w;
        for (int i = num; i < static_cast<int>(prototype.sides_.size()); ++i)
            w.push_back(prototype.sides_[i].get());
        w.push_back(&closing);

        int a = 0, b = 0;
        if (!WallsValid(w, true, {static_cast<int>(w.size()) - 1}, &a, &b)) {
            Notify(NoteLevel::Error,
                   QString("Нельзя замкнуть комнату: замыкающая стена пересекает или касается стены %1.")
                       .arg(a + num + 1));
            return;
        }
    }

    // Луч: у стёртых стен он удаляется, у остальных сдвигаются индексы
    if (prototype.isBeamSet_) {
        if (prototype.beamWall_ < num) {
            prototype.isBeamSet_ = false;
            Notify(NoteLevel::Warning,
                   "Источник луча удалён: стена, на которой он стоял, исчезла при соединении точек.");
        } else {
            prototype.beamWall_ -= num;
        }
    }

    prototype.sides_.erase(
        prototype.sides_.begin(),
        prototype.sides_.begin() + num
        );
    prototype.first = start;

    prototype.sides_.push_back(std::make_unique<PlaneMirror>(end, start));
    prototype.isCycled_ = true;

    Notify(NoteLevel::Info,
           QString("Комната замкнута, сторон: %1.")
               .arg(static_cast<int>(prototype.sides_.size())));
}

void FrontEndOperators::DeleteLast() {
    if (prototype.sides_.empty()) {
        if (prototype.isExist()) {
            prototype.isExist_ = false;
            Notify(NoteLevel::Info, "Первая точка удалена.");
        } else {
            Notify(NoteLevel::Warning, "Откатывать нечего: точек нет.");
        }
    } else {
        const int last = static_cast<int>(prototype.sides_.size()) - 1;
        const bool wasCycled = prototype.isCycled_;

        if (prototype.isBeamSet_ && prototype.beamWall_ == last) {
            prototype.isBeamSet_ = false;
            Notify(NoteLevel::Warning,
                   "Источник луча удалён вместе со стеной, на которой он стоял.");
        }

        prototype.sides_.pop_back();
        prototype.isCycled_ = false;

        Notify(NoteLevel::Info,
               wasCycled ? "Замыкающая стена удалена: комната снова не замкнута."
                         : "Последняя точка удалена.");
    }
}

void FrontEndOperators::ChangeMirror(
    int num,
    const ChangePointData &data)
{
    auto &sides = prototype.sides_;
    const int n = static_cast<int>(sides.size());
    const bool cycled = prototype.isCycled_;

    if (n == 0 || num < 0 || num > n || (cycled && num == n)) {
        Notify(NoteLevel::Error, "Не удалось изменить точку: такой точки нет.");
        return;
    }

    // Последняя точка незамкнутого полигона - конец последней стены
    const bool lastPoint = (!cycled && num == n);
    const Points oldPoint = lastPoint ? sides.back()->getEnd()
                                      : sides[num]->getStart();
    const int pointNo = num + 1;

    // ---------------------------------------------------------
    // 1. Новые координаты: вне допустимой зоны - оставляем прежние
    // ---------------------------------------------------------
    double newX = data.x;
    double newY = data.y;

    if (!(newX >= Limits::MinX && newX <= Limits::MaxX)) {
        Notify(NoteLevel::Error,
               QString("Координата X = %1 вне допустимой зоны (от %2 до %3): оставлена прежней (%4).")
                   .arg(Fmt(data.x), Fmt(Limits::MinX), Fmt(Limits::MaxX), Fmt(oldPoint.getX())));
        newX = oldPoint.getX();
    }
    if (!(newY >= Limits::MinY && newY <= Limits::MaxY)) {
        Notify(NoteLevel::Error,
               QString("Координата Y = %1 вне допустимой зоны (от %2 до %3): оставлена прежней (%4).")
                   .arg(Fmt(data.y), Fmt(Limits::MinY), Fmt(Limits::MaxY), Fmt(oldPoint.getY())));
        newY = oldPoint.getY();
    }

    Points newPoint(newX, newY);

    // ---------------------------------------------------------
    // 2. Слишком близко к другой вершине - координаты не меняем
    // ---------------------------------------------------------
    const int vertexCount = cycled ? n : n + 1;
    for (int j = 0; j < vertexCount; ++j) {
        if (j == num) continue;

        const Points v = (j < n) ? sides[j]->getStart()
                                 : sides.back()->getEnd();

        if (v.Dist(newPoint) <= Limits::MinPointDist) {
            Notify(NoteLevel::Error,
                   QString("Точка %1 слишком близко к точке %2 (ближе %3 пикселей): координаты не изменены.")
                       .arg(pointNo)
                       .arg(j + 1)
                       .arg(static_cast<int>(Limits::MinPointDist)));
            newPoint = oldPoint;
            break;
        }
    }

    // ---------------------------------------------------------
    // 3. Собираем новые стены, пока ничего не меняя в полигоне
    // ---------------------------------------------------------
    struct Item {
        int idx;
        std::unique_ptr<Mirror> mirror;
        bool flattened;
        double halfChord;
        double radius;
    };
    std::vector<Item> items;

    // Стена сохраняет свой тип и радиус, меняются только концы
    auto addKeep = [&](int idx, const Points &a, const Points &b) {
        Item it{idx, nullptr, false, a.Dist(b) / 2.0, sides[idx]->getRadius()};
        it.mirror = MakeMirror(TypeOf(*sides[idx]), a, b,
                               sides[idx]->getRadius(), &it.flattened);
        items.push_back(std::move(it));
    };

    // Стена получает тип и радиус из диалога
    auto addNew = [&](int idx, const Points &a, const Points &b) {
        Item it{idx, nullptr, false, a.Dist(b) / 2.0, data.radius};
        it.mirror = MakeMirror(data.mirrorType, a, b, data.radius, &it.flattened);
        items.push_back(std::move(it));
    };

    try {
        if (lastPoint) {
            addKeep(n - 1, sides[n - 1]->getStart(), newPoint);
        } else if (cycled && num == 0) {
            addNew(0, newPoint, sides[0]->getEnd());
            addKeep(n - 1, sides[n - 1]->getStart(), newPoint);
        } else {
            addNew(num, newPoint, sides[num]->getEnd());
            if (num > 0)
                addKeep(num - 1, sides[num - 1]->getStart(), newPoint);
        }
    } catch (const std::exception &e) {
        Notify(NoteLevel::Error,
               QString("Не удалось изменить точку %1: %2")
                   .arg(pointNo)
                   .arg(QString::fromUtf8(e.what())));
        return;
    }

    // ---------------------------------------------------------
    // 3б. Новые стены не должны пересекаться с остальными
    // ---------------------------------------------------------
    {
        std::vector<const Mirror *> w(n);
        std::vector<int> changed;

        for (int i = 0; i < n; ++i)
            w[i] = sides[i].get();
        for (const Item &it : items) {
            w[it.idx] = it.mirror.get();
            changed.push_back(it.idx);
        }

        int a = 0, b = 0;
        if (!WallsValid(w, cycled, changed, &a, &b)) {
            Notify(NoteLevel::Error,
                   QString("Изменение точки %1 отменено: зеркало %2 начнёт пересекаться или касаться зеркала %3.")
                       .arg(pointNo)
                       .arg(a + 1)
                       .arg(b + 1));
            return;
        }
    }

    // ---------------------------------------------------------
    // 4. Луч стоял на изменяемой стене - он удаляется
    // ---------------------------------------------------------
    if (prototype.isBeamSet_) {
        for (const Item &it : items) {
            if (it.idx == prototype.beamWall_) {
                prototype.isBeamSet_ = false;
                Notify(NoteLevel::Warning,
                       "Источник луча удалён: стена, на которой он стоял, изменена.");
                break;
            }
        }
    }

    // ---------------------------------------------------------
    // 5. Применяем изменения
    // ---------------------------------------------------------
    for (Item &it : items)
        sides[it.idx] = std::move(it.mirror);

    if (num == 0)
        prototype.first = newPoint;

    // ---------------------------------------------------------
    // 6. Сообщения
    // ---------------------------------------------------------
    for (const Item &it : items) {
        if (it.flattened) {
            Notify(NoteLevel::Warning,
                   QString("Зеркало %1: радиус %2 меньше половины его длины (%3), зеркало оставлено плоским.")
                       .arg(it.idx + 1)
                       .arg(Fmt(it.radius), Fmt(it.halfChord)));
        }
    }

    if (newPoint.Dist(oldPoint) > 1e-9) {
        Notify(NoteLevel::Info,
               QString("Точка %1 перемещена: %2 -> %3.")
                   .arg(pointNo)
                   .arg(PointStr(oldPoint), PointStr(newPoint)));
    }

    if (!lastPoint) {
        Notify(NoteLevel::Info,
               QString("Зеркало %1: %2.")
                   .arg(items[0].idx + 1)
                   .arg(TypeName(*sides[items[0].idx])));
    }
}

bool FrontEndOperators::NextReflection()
{
    if (!prototype.isBeamSet_) {
        Notify(NoteLevel::Error, "Сначала поставьте источник луча.");
        return false;
    }

    if (looped_) {
        Notify(NoteLevel::Warning,
               "Луч уже зациклился: дальше отражения только повторяются.");
        return false;
    }

    // Первое нажатие: идём от стартового луча полигона
    if (history_.size() <= 1) {
        tempBeam_ = prototype.beam_;
        history_.assign(1, Points(tempBeam_.getX(), tempBeam_.getY()));
        states_.assign(1, tempBeam_);
    }

    auto result = prototype.TraceBeamIter(tempBeam_);
    if (result.second.Dist(history_.back()) < 1e-9) {
        Notify(NoteLevel::Error,
               "Не удалось найти следующее отражение: луч никуда не попал.");
        return false;
    }

    tempBeam_ = result.first;
    history_.push_back(result.second);
    states_.push_back(tempBeam_);

    const int step = static_cast<int>(history_.size()) - 1;

    Notify(NoteLevel::Info,
           QString("Отражение %1: %2.").arg(step).arg(PointStr(result.second)));

    // Такая точка с таким направлением уже была - ход луча повторяется
    for (int m = 0; m < step; ++m) {
        if (SameState(states_[m], tempBeam_)) {
            looped_ = true;
            Notify(NoteLevel::Warning,
                   QString("Луч света зациклился: после отражения %1 ход луча повторяется "
                           "(период - %2). Дальше отражаться нечего: измените угол луча "
                           "(клик по красной точке) или переставьте источник.")
                       .arg(step)
                       .arg(step - m));
            break;
        }
    }

    return true;
}

void FrontEndOperators::ResetTrace()
{
    history_.clear();
    states_.clear();
    looped_ = false;

    if (prototype.isBeamSet_) {
        history_.push_back(Points(prototype.beam_.getX(), prototype.beam_.getY()));
        states_.push_back(prototype.beam_);
    }
}
