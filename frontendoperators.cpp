#include "frontendoperators.h"
#include <QWidget>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <locale>
#include <limits>
#include "constants.h"
#include "beamangledialog.h"   // BeamAngle::Min / Max / Decimals

using namespace FeEps;
namespace {

// Тип зеркала (см. MirrorKind)
int TypeOf(const Mirror &m)
{
    if (dynamic_cast<const ConcaveMirror *>(&m)) return MirrorKind::Concave;
    if (dynamic_cast<const ConvexMirror *>(&m))  return MirrorKind::Convex;
    return MirrorKind::Plane;
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

    if (type != MirrorKind::Plane && !(radius >= a.Dist(b) / 2.0)) {
        if (flattened) *flattened = true;
        type = MirrorKind::Plane;
    }

    switch (type) {
    case MirrorKind::Concave:  return std::make_unique<ConcaveMirror>(a, b, radius);
    case MirrorKind::Convex:   return std::make_unique<ConvexMirror>(a, b, radius);
    default: return std::make_unique<PlaneMirror>(a, b);
    }
}

// Одинаковое состояние луча: та же точка и то же направление
bool SameState(const Beam &a, const Beam &b)
{
    const double posEps = 1e-3;     // пиксели
    const double angEps = 1e-5;     // радианы

    if (std::abs(a.getX() - b.getX()) > posEps ||
        std::abs(a.getY() - b.getY()) > posEps)
        return false;

    return std::abs(std::remainder(a.getA() - b.getA(), Math::TwoPi)) <= angEps;
}

QString Fmt(double v)
{
    return QString::number(v, 'f', Style::MessageDecimals);
}

QString PointStr(const Points &p)
{
    return QString("(%1; %2)").arg(Fmt(p.getX()), Fmt(p.getY()));
}

QString TypeName(const Mirror &m)
{
    switch (TypeOf(m)) {
    case MirrorKind::Concave:  return QString("вогнутое, R = %1").arg(Fmt(m.getRadius()));
    case MirrorKind::Convex:   return QString("выпуклое, R = %1").arg(Fmt(m.getRadius()));
    default: return QString("плоское");
    }
}

// Точка зеркала по параметру t от 0 (начало) до 1 (конец)
Points PointAt(const Mirror &m, double t)
{
    const Points s = m.getStart();
    const Points e = m.getEnd();

    if (m.isFlat())
        return Points(s.getX() + t * (e.getX() - s.getX()),
                      s.getY() + t * (e.getY() - s.getY()));

    const Points c = m.CircleCenter();
    const double r = m.getRadius();

    const double a0 = std::atan2(s.getY() - c.getY(), s.getX() - c.getX());
    const double a1 = std::atan2(e.getY() - c.getY(), e.getX() - c.getX());

    // Меньшая дуга; центр слева от хорды - обход против часовой, справа - по часовой
    double delta = std::abs(std::remainder(a1 - a0, Math::TwoPi));
    if (m.getRad() < 0) delta = -delta;

    const double ang = a0 + t * delta;
    return Points(c.getX() + r * std::cos(ang), c.getY() + r * std::sin(ang));
}

// Наименьшее расстояние между двумя зеркалами: точки одного берутся
// с шагом, до другого считается точное расстояние (концы входят в выборку)
double WallDistance(const Mirror &a, const Mirror &b)
{
    double best = std::numeric_limits<double>::max();

    auto scan = [&](const Mirror &from, const Mirror &to) {
        for (int k = 0; k <= WallScanSteps; ++k) {
            double d = 0.0;
            to.ClosestPoint(PointAt(from, static_cast<double>(k) / WallScanSteps), d);
            best = std::min(best, d);
        }
    };

    scan(a, b);
    scan(b, a);
    return best;
}

// Соседние стены first -> second (конец first = начало second).
// Они не должны слипаться: дальние концы не ближе толщины линии к соседней стене.
bool AdjacentTooClose(const Mirror &first, const Mirror &second)
{
    double d = 0.0;
    second.ClosestPoint(first.getStart(), d);
    if (d < Style::WallWidth) return true;

    first.ClosestPoint(second.getEnd(), d);
    return d < Style::WallWidth;
}

enum class WallCheck { Ok, Cross, TooClose };

// Проверка набора стен: стены не должны пересекаться и касаться друг друга
// нигде, кроме общей вершины соседних стен (как Polygon::Check, но работает
// и для разомкнутой цепочки), а линии толщиной Style::WallWidth не должны
// сливаться. only - индексы изменённых стен: проверяются только пары с их
// участием (пусто - все пары). При ошибке в *a и *b - индексы стен (a < b).
WallCheck CheckWalls(const std::vector<const Mirror *> &w,
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

            const bool wrap = closed && i == 0 && j == n - 1;
            const bool adjacent = (j == i + 1) || wrap;

            // 1. Геометрические пересечения (кроме общей вершины соседей)
            std::vector<Points> allowed;
            if (j == i + 1) allowed.push_back(w[i]->getEnd());
            if (wrap)       allowed.push_back(w[j]->getEnd());

            for (const Points &p : w[i]->Intersections(*w[j])) {
                bool ok = false;
                for (const Points &q : allowed) {
                    if (p.Dist(q) < SamePointEps) { ok = true; break; }
                }
                if (!ok) {
                    if (a) *a = i;
                    if (b) *b = j;
                    return WallCheck::Cross;
                }
            }

            // 2. Толщина линий
            bool tooClose;
            if (adjacent) {
                const Mirror &first  = wrap ? *w[j] : *w[i];
                const Mirror &second = wrap ? *w[i] : *w[j];
                tooClose = AdjacentTooClose(first, second);
            } else {
                tooClose = WallDistance(*w[i], *w[j]) < Style::WallWidth;
            }

            if (tooClose) {
                if (a) *a = i;
                if (b) *b = j;
                return WallCheck::TooClose;
            }
        }
    }
    return WallCheck::Ok;
}

} // namespace

// ============================================================
// Загрузка комнаты из файла: разбор строк
// ============================================================
namespace {

using U32 = std::u32string;

bool IsSpaceCh(char32_t c)  { return c == U' ' || c == U'\t' || c == 0x00A0 || c == 0xFEFF; }
bool IsDigitCh(char32_t c)  { return c >= U'0' && c <= U'9'; }
// буква строчная (текст приводится к нижнему регистру заранее)
bool IsLetterCh(char32_t c) { return (c >= U'a' && c <= U'z') || (c >= 0x0430 && c <= 0x044F) || c == 0x0451; }

// Нижний регистр для латиницы и кириллицы
char32_t LowerCh(char32_t c)
{
    if (c >= U'A' && c <= U'Z')       return c + 32;
    if (c >= 0x0410 && c <= 0x042F)   return c + 0x20;   // А-Я -> а-я
    if (c >= 0x0400 && c <= 0x040F)   return c + 0x50;   // Ё и др. -> ё и др.
    return c;
}

U32 Lowered(const U32 &s)
{
    U32 r = s;
    for (char32_t &c : r) c = LowerCh(c);
    return r;
}

// UTF-8 -> UTF-32; повреждённые байты заменяются на U+FFFD
U32 DecodeUtf8(const std::string &s)
{
    U32 out;
    out.reserve(s.size());
    const size_t n = s.size();
    size_t i = 0;
    while (i < n) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        char32_t cp = 0xFFFD;
        size_t extra = 0;
        if (c < 0x80)                  { cp = c; }
        else if ((c & 0xE0) == 0xC0)   { extra = 1; cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0)   { extra = 2; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0)   { extra = 3; cp = c & 0x07; }

        size_t step = 1;
        if (extra > 0) {
            bool ok = (i + extra < n);
            for (size_t k = 1; ok && k <= extra; ++k) {
                const unsigned char cc = static_cast<unsigned char>(s[i + k]);
                if ((cc & 0xC0) != 0x80) ok = false;
                else cp = (cp << 6) | (cc & 0x3F);
            }
            if (ok) step = extra + 1;
            else    cp = 0xFFFD;
        }
        out.push_back(cp);
        i += step;
    }
    return out;
}

// Разделение на строки (\n, \r\n и \r)
std::vector<U32> SplitLines(const U32 &t)
{
    std::vector<U32> lines;
    U32 cur;
    for (size_t i = 0; i < t.size(); ++i) {
        const char32_t c = t[i];
        if (c == U'\n' || c == U'\r') {
            if (c == U'\r' && i + 1 < t.size() && t[i + 1] == U'\n') ++i;
            lines.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    lines.push_back(cur);
    return lines;
}

size_t SkipSpaces(const U32 &s, size_t p)
{
    while (p < s.size() && IsSpaceCh(s[p])) ++p;
    return p;
}

bool StartsWith(const U32 &s, size_t p, const U32 &lit)
{
    return p <= s.size() && s.compare(p, lit.size(), lit) == 0;
}

// Есть ли в строке (уже в нижнем регистре) фраза «слово1 слово2 ... :»:
// слова через пробелы, двоеточие после необязательных пробелов, перед первым словом не буква
bool HasPhrase(const U32 &low, const std::vector<U32> &words)
{
    for (size_t start = 0; start < low.size(); ++start) {
        if (start > 0 && IsLetterCh(low[start - 1])) continue;

        size_t p = start;
        bool ok = true;
        for (size_t w = 0; w < words.size(); ++w) {
            if (w > 0) {
                const size_t before = p;
                p = SkipSpaces(low, p);
                if (p == before) { ok = false; break; }
            }
            if (!StartsWith(low, p, words[w])) { ok = false; break; }
            p += words[w].size();
        }
        if (!ok) continue;

        p = SkipSpaces(low, p);
        if (p < low.size() && low[p] == U':') return true;
    }
    return false;
}

// Число с позиции pos: цифры, затем (необязательно) точка или запятая и цифры.
// Знак не учитывается: минус - посторонний символ. false - на pos нет цифры.
bool ParseNumberAt(const U32 &s, size_t pos, double &value, size_t &end)
{
    if (pos >= s.size() || !IsDigitCh(s[pos])) return false;

    std::string txt;
    size_t p = pos;
    while (p < s.size() && IsDigitCh(s[p])) txt.push_back(static_cast<char>(s[p++]));
    if (p + 1 < s.size() && (s[p] == U'.' || s[p] == U',') && IsDigitCh(s[p + 1])) {
        txt.push_back('.');
        ++p;
        while (p < s.size() && IsDigitCh(s[p])) txt.push_back(static_cast<char>(s[p++]));
    }

    std::istringstream iss(txt);
    iss.imbue(std::locale::classic());   // не зависит от настроек системы
    double v = 0.0;
    if (!(iss >> v)) v = std::numeric_limits<double>::infinity();   // слишком длинное число

    value = v;
    end = p;
    return true;
}

// Все числа строки по порядку
std::vector<double> ExtractNumbers(const U32 &s)
{
    std::vector<double> r;
    size_t p = 0;
    while (p < s.size()) {
        double v = 0.0;
        size_t e = p;
        if (ParseNumberAt(s, p, v, e)) { r.push_back(v); p = e; }
        else                           { ++p; }
    }
    return r;
}

// «тип: вогнутое(N)» / «тип: выпуклое(N)» / «тип: выгнутое(N)» с позиции pos (строка в нижнем регистре).
// Радиус обязан стоять в скобках сразу после названия типа, иначе фраза не считается.
bool MatchTypePhrase(const U32 &low, size_t pos, int &type, double &radius)
{
    size_t p = pos;
    if (!StartsWith(low, p, U"тип")) return false;
    p += 3;
    p = SkipSpaces(low, p);
    if (p >= low.size() || low[p] != U':') return false;
    ++p;
    p = SkipSpaces(low, p);

    int t = MirrorKind::Plane;
    if      (StartsWith(low, p, U"вогнут")) t = MirrorKind::Concave;
    else if (StartsWith(low, p, U"выпукл")) t = MirrorKind::Convex;
    else if (StartsWith(low, p, U"выгнут")) t = MirrorKind::Convex;
    else return false;
    p += 6;

    if (StartsWith(low, p, U"ое") || StartsWith(low, p, U"ая")) p += 2;
    else return false;

    p = SkipSpaces(low, p);
    if (p >= low.size() || low[p] != U'(') return false;
    ++p;
    p = SkipSpaces(low, p);

    double r = 0.0;
    size_t e = p;
    if (!ParseNumberAt(low, p, r, e)) return false;
    p = SkipSpaces(low, e);
    if (p >= low.size() || low[p] != U')') return false;

    type = t;
    radius = r;
    return true;
}

struct RoomLine
{
    double x = 0.0;
    double y = 0.0;
    int    type = MirrorKind::Plane;   // тип стены, которая идёт после этой точки
    double radius = 0.0;
};

// Значащая строка с углом: ровно два числа (координаты), после них - необязательное
// «тип: ...(N)». false - строка незначащая.
bool ParseRoomLine(const U32 &line, RoomLine &out)
{
    const U32 low = Lowered(line);

    size_t phrasePos = U32::npos;
    int type = MirrorKind::Plane;
    double radius = 0.0;
    for (size_t i = 0; i < low.size(); ++i) {
        if (low[i] == U'т' && MatchTypePhrase(low, i, type, radius)) { phrasePos = i; break; }
    }

    // Фраза про тип учитывается только строго после двух чисел; без неё (или с неверной
    // записью) она просто текст, и числа считаются по всей строке
    const U32 coordPart = (phrasePos == U32::npos) ? line : line.substr(0, phrasePos);
    const std::vector<double> nums = ExtractNumbers(coordPart);
    if (nums.size() != 2) return false;

    out.x = nums[0];
    out.y = nums[1];
    if (phrasePos != U32::npos) {
        out.type = type;
        out.radius = radius;
    }
    return true;
}

QString FmtPrecise(double v)
{
    return QString::number(v, 'f', Style::PreciseDecimals);
}

} // namespace

FrontEndOperators::FrontEndOperators() {}

void FrontEndOperators::Notify(NoteLevel level, const QString &text) const
{
    if (notify) notify(level, text);
}

void FrontEndOperators::CreatePoint(double x, double y) {
    // До замыкания стен на одну меньше: замыкающая добавится при замыкании комнаты
    if (static_cast<int>(prototype.sides_.size()) >= Limits::MaxWalls - 1) {
        Notify(NoteLevel::Warning,
               QString("Достигнут предел: в комнате не больше %1 сторон. "
                       "Новую точку поставить нельзя, замкните комнату.")
                   .arg(Limits::MaxWalls));
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
        const WallCheck res = CheckWalls(w, false, {static_cast<int>(w.size()) - 1}, &a, &b);
        if (res == WallCheck::Cross) {
            Notify(NoteLevel::Error,
                   QString("Нельзя поставить точку: новая стена пересекает или касается стены %1.")
                       .arg(a + 1));
            return;
        }
        if (res == WallCheck::TooClose) {
            Notify(NoteLevel::Error,
                   QString("Нельзя поставить точку: новая стена слишком близко к стене %1 "
                           "(ближе толщины линии, %2 пикселей).")
                       .arg(a + 1)
                       .arg(Style::WallWidth));
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

    // Комната должна остаться многоугольником: не меньше Limits::MinWalls сторон
    // (первые num стен при соединении удаляются, добавляется замыкающая)
    const int resultWalls = static_cast<int>(prototype.sides_.size()) - num + 1;
    if (resultWalls < Limits::MinWalls) {
        Notify(NoteLevel::Error,
               QString("Нельзя замкнуть комнату: получится сторон: %1, а нужно от %2 до %3. "
                       "Подключите более раннюю точку или поставьте ещё точки.")
                   .arg(resultWalls)
                   .arg(Limits::MinWalls)
                   .arg(Limits::MaxWalls));
        return;
    }

    Points start = prototype.sides_[num]->getStart();
    Points end = prototype.sides_.back()->getEnd();

    // Замыкающая стена нулевой длины невозможна
    if (start.Dist(end) < SamePointEps) {
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
        const WallCheck res = CheckWalls(w, true, {static_cast<int>(w.size()) - 1}, &a, &b);
        if (res == WallCheck::Cross) {
            Notify(NoteLevel::Error,
                   QString("Нельзя замкнуть комнату: замыкающая стена пересекает или касается стены %1.")
                       .arg(a + num + 1));
            return;
        }
        if (res == WallCheck::TooClose) {
            Notify(NoteLevel::Error,
                   QString("Нельзя замкнуть комнату: замыкающая стена слишком близко к стене %1 "
                           "(ближе толщины линии, %2 пикселей).")
                       .arg(a + num + 1)
                       .arg(Style::WallWidth));
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
            Notify(NoteLevel::Info,
                   QString("Первая точка 1 %1 удалена.").arg(PointStr(prototype.first)));
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

        const Points lastPoint = prototype.sides_.back()->getEnd();
        const int lastPointNo = static_cast<int>(prototype.sides_.size()) + 1;

        prototype.sides_.pop_back();
        prototype.isCycled_ = false;

        Notify(NoteLevel::Info,
               wasCycled ? QString("Замыкающая стена удалена: комната снова не замкнута.")
                         : QString("Последняя точка %1 %2 удалена.")
                               .arg(lastPointNo)
                               .arg(PointStr(lastPoint)));
    }
}

void FrontEndOperators::DeletePoint(int num)
{
    auto &sides = prototype.sides_;
    const int n = static_cast<int>(sides.size());
    const bool cycled = prototype.isCycled_;

    // Замкнутая комната: вершин ровно столько же, сколько стен (num: 0..n-1)
    if (!prototype.isExist() || num < 0 || num > n || (cycled && num >= n)) {
        Notify(NoteLevel::Error, "Не удалось удалить точку: такой точки нет.");
        return;
    }

    // После удаления в замкнутой комнате должно остаться не меньше Limits::MinWalls вершин
    if (cycled && n - 1 < Limits::MinWalls) {
        Notify(NoteLevel::Error,
               QString("Нельзя удалить точку: в замкнутой комнате должно остаться не меньше %1 вершин "
                       "(сейчас их %2).")
                   .arg(Limits::MinWalls)
                   .arg(n));
        return;
    }

    const int pointNo = num + 1;
    const Points oldPoint = (num < n)  ? sides[num]->getStart()
                            : (n > 0)  ? sides.back()->getEnd()
                                      : prototype.first;

    // Если на исчезающей стене (с нуля) стоял луч - он удаляется
    auto dropBeamIfOn = [&](int lo, int hi) {
        if (prototype.isBeamSet_ && prototype.beamWall_ >= lo && prototype.beamWall_ <= hi) {
            prototype.isBeamSet_ = false;
            Notify(NoteLevel::Warning,
                   "Источник луча удалён вместе со стеной, на которой он стоял.");
        }
    };

    if (!cycled) {
        // 1. Единственная точка: комнаты больше нет
        if (n == 0) {
            prototype.isExist_ = false;
            prototype.first = Points();
            Notify(NoteLevel::Info,
                   QString("Единственная точка 1 %1 удалена.").arg(PointStr(oldPoint)));
            return;
        }

        // 2. Последняя точка: исчезает последняя стена
        if (num == n) {
            dropBeamIfOn(n - 1, n - 1);
            sides.pop_back();
            Notify(NoteLevel::Info,
                   QString("Точка %1 %2 удалена вместе со стеной %3.")
                       .arg(pointNo)
                       .arg(PointStr(oldPoint))
                       .arg(n));
            return;
        }

        // 3. Первая точка: исчезает первая стена, первой становится её конец
        if (num == 0) {
            const Points newFirst = sides[0]->getEnd();
            dropBeamIfOn(0, 0);
            if (prototype.isBeamSet_)
                --prototype.beamWall_;   // все стены сдвигаются на одну
            sides.erase(sides.begin());
            prototype.first = newFirst;
            Notify(NoteLevel::Info,
                   QString("Точка 1 %1 удалена вместе со стеной 1. Первой стала бывшая точка 2 %2.")
                       .arg(PointStr(oldPoint))
                       .arg(PointStr(newFirst)));
            return;
        }
    }

    // 4. Вершина с двумя соседями (средняя точка незамкнутой комнаты или любая
    //    вершина замкнутой): стены prev и num заменяются одной плоской стеной между соседями
    const int prev = (num - 1 + n) % n;
    const Points a = sides[prev]->getStart();
    const Points b = sides[num]->getEnd();

    std::unique_ptr<PlaneMirror> merged;
    try {
        merged = std::make_unique<PlaneMirror>(a, b);
    } catch (const std::exception &e) {
        Notify(NoteLevel::Error,
               QString("Не удалось удалить точку %1: %2")
                   .arg(pointNo)
                   .arg(QString::fromUtf8(e.what())));
        return;
    }

    // Набор стен после удаления: oldIdx[k] - прежний индекс стены (-1 - новая стена)
    std::vector<const Mirror *> w;
    std::vector<int> oldIdx;
    int mergedPos = 0;

    if (cycled && num == 0) {
        // стена 0 исчезает, на месте последней стоит новая (она заканчивается там же, где кончалась стена 0)
        for (int i = 1; i < n - 1; ++i) {
            w.push_back(sides[i].get());
            oldIdx.push_back(i);
        }
        mergedPos = static_cast<int>(w.size());
        w.push_back(merged.get());
        oldIdx.push_back(-1);
    } else {
        for (int i = 0; i < n; ++i) {
            if (i == num) continue;
            if (i == num - 1) {
                mergedPos = static_cast<int>(w.size());
                w.push_back(merged.get());
                oldIdx.push_back(-1);
            } else {
                w.push_back(sides[i].get());
                oldIdx.push_back(i);
            }
        }
    }

    // Номера соседних точек для сообщений (с единицы)
    const int prevPointNo = prev + 1;
    const int nextPointNo = cycled ? (num + 1) % n + 1 : num + 2;

    int x = 0, y = 0;
    const WallCheck res = CheckWalls(w, cycled, {mergedPos}, &x, &y);
    if (res != WallCheck::Ok) {
        const int other = (x == mergedPos) ? y : x;
        const int otherNo = oldIdx[other] + 1;   // номер в прежней нумерации

        if (res == WallCheck::Cross) {
            Notify(NoteLevel::Error,
                   QString("Нельзя удалить точку %1: новая стена между точками %2 и %3 "
                           "пересечёт или коснётся стены %4.")
                       .arg(pointNo)
                       .arg(prevPointNo)
                       .arg(nextPointNo)
                       .arg(otherNo));
        } else {
            Notify(NoteLevel::Error,
                   QString("Нельзя удалить точку %1: новая стена между точками %2 и %3 "
                           "окажется слишком близко к стене %4 (ближе толщины линии, %5 пикселей).")
                       .arg(pointNo)
                       .arg(prevPointNo)
                       .arg(nextPointNo)
                       .arg(otherNo)
                       .arg(Style::WallWidth));
        }
        return;
    }

    // Применяем
    dropBeamIfOn(prev, prev);
    dropBeamIfOn(num, num);

    if (prototype.isBeamSet_) {
        // индекс стены с лучом в новой нумерации
        for (int j = 0; j < static_cast<int>(oldIdx.size()); ++j) {
            if (oldIdx[j] == prototype.beamWall_) {
                prototype.beamWall_ = j;
                break;
            }
        }
    }

    std::vector<std::unique_ptr<Mirror>> result;
    result.reserve(oldIdx.size());
    for (int k : oldIdx) {
        if (k < 0) result.push_back(std::move(merged));
        else       result.push_back(std::move(sides[k]));
    }
    sides = std::move(result);

    if (cycled && num == 0)
        prototype.first = sides[0]->getStart();   // первой стала прежняя вторая вершина

    Notify(NoteLevel::Info,
           QString("Точка %1 %2 удалена. Стены %3 и %4 заменены одной плоской стеной между "
                   "бывшими точками %5 %6 и %7 %8.")
               .arg(pointNo)
               .arg(PointStr(oldPoint))
               .arg(prev + 1)
               .arg(num + 1)
               .arg(prevPointNo)
               .arg(PointStr(a))
               .arg(nextPointNo)
               .arg(PointStr(b)));
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
        const WallCheck res = CheckWalls(w, cycled, changed, &a, &b);
        if (res == WallCheck::Cross) {
            Notify(NoteLevel::Error,
                   QString("Изменение точки %1 отменено: зеркала %2 и %3 начнут пересекаться или касаться.")
                       .arg(pointNo)
                       .arg(a + 1)
                       .arg(b + 1));
            return;
        }
        if (res == WallCheck::TooClose) {
            Notify(NoteLevel::Error,
                   QString("Изменение точки %1 отменено: зеркала %2 и %3 окажутся слишком близко "
                           "(ближе толщины линии, %4 пикселей).")
                       .arg(pointNo)
                       .arg(a + 1)
                       .arg(b + 1)
                       .arg(Style::WallWidth));
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

    if (newPoint.Dist(oldPoint) > NoMoveEps) {
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

// Запоминает комнату (вершины, типы и радиусы стен) и параметры источника луча
TraceSnapshot FrontEndOperators::MakeSnapshot() const
{
    TraceSnapshot s;

    for (const auto &m : prototype.sides_) {
        TraceSnapshot::Wall w;
        w.x = m->getStart().getX();
        w.y = m->getStart().getY();
        w.type = TypeOf(*m);
        w.radius = std::abs(m->getRadius());
        s.walls.push_back(w);
    }

    s.beamWall = static_cast<int>(prototype.beamWall_);
    s.beamWallAngle = prototype.beamWallAngle_;
    return s;
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

    // Первое отражение (или первое после сброса): только теперь стираем прежнюю
    // историю и идём от стартового луча полигона
    if (traceCleared_ || history_.size() <= 1) {
        tempBeam_ = prototype.beam_;
        history_.assign(1, Points(tempBeam_.getX(), tempBeam_.getY()));
        states_.assign(1, tempBeam_);
        snapshot_ = MakeSnapshot();
        traceCleared_ = false;
    }

    auto result = prototype.TraceBeamIter(tempBeam_);
    if (result.second.Dist(history_.back()) < NoMoveEps) {
        Notify(NoteLevel::Error,
               "Не удалось найти следующее отражение: луч никуда не попал.");
        return false;
    }

    tempBeam_ = result.first;
    history_.push_back(result.second);
    states_.push_back(tempBeam_);

    const int step = static_cast<int>(history_.size()) - 1;

    Notify(NoteLevel::Reflection,
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
    // Путь исчезает с экрана сразу (traceCleared_), но history_ и states_ остаются
    // нетронутыми до первого нового отражения (см. NextReflection)
    traceCleared_ = true;
    looped_ = false;
}


LoadResult FrontEndOperators::LoadRoom(const std::string &utf8Text)
{
    U32 text = DecodeUtf8(utf8Text);
    if (!text.empty() && text[0] == 0xFEFF)
        text.erase(0, 1);   // метка порядка байтов UTF-8

    bool blank = true;
    for (char32_t c : text) {
        if (!IsSpaceCh(c) && c != U'\n' && c != U'\r') { blank = false; break; }
    }
    if (blank) {
        Notify(NoteLevel::Error, "Файл пуст: загружать нечего.");
        return LoadResult::Failed;
    }

    const std::vector<U32> lines = SplitLines(text);
    const int lineCount = static_cast<int>(lines.size());

    // Строки с фразами (берётся первая такая строка)
    int roomPhrase = -1;
    int beamPhrase = -1;
    for (int i = 0; i < lineCount; ++i) {
        const U32 low = Lowered(lines[i]);
        if (roomPhrase < 0 && HasPhrase(low, {U"углы", U"комнаты"})) roomPhrase = i;
        if (beamPhrase < 0 && HasPhrase(low, {U"луч"}))              beamPhrase = i;
    }
    if (roomPhrase < 0 && beamPhrase < 0) {
        Notify(NoteLevel::Error,
               "В файле нет ни фразы «Углы комнаты:», ни фразы «Луч:»: загружать нечего.");
        return LoadResult::Failed;
    }

    // ---------- Этап 1: структура файла (ошибки собираются все сразу) ----------
    std::vector<QString> errors;

    std::vector<RoomLine> room;
    int skippedLines = 0;
    if (roomPhrase < 0) {
        errors.push_back("В файле нет фразы «Углы комнаты:».");
    } else {
        int i = roomPhrase + 1;
        for (; i < lineCount && static_cast<int>(room.size()) < Limits::MaxWalls; ++i) {
            RoomLine rl;
            if (!ParseRoomLine(lines[i], rl)) break;   // первая незначащая строка обрывает список
            room.push_back(rl);
        }
        if (static_cast<int>(room.size()) == Limits::MaxWalls) {
            for (; i < lineCount; ++i) {
                RoomLine extra;
                if (!ParseRoomLine(lines[i], extra)) break;
                ++skippedLines;
            }
        }
        if (static_cast<int>(room.size()) < Limits::MinWalls) {
            errors.push_back(
                QString("После фразы «Углы комнаты:» найдено значащих строк: %1, а нужно не меньше %2 "
                        "(не больше %3). Значащая строка - ровно два числа (координаты x и y), после них "
                        "можно дописать «тип: вогнутое(R)» или «тип: выпуклое(R)». Такие строки должны "
                        "идти подряд сразу после фразы.")
                    .arg(static_cast<int>(room.size()))
                    .arg(Limits::MinWalls)
                    .arg(Limits::MaxWalls));
        }
    }

    bool haveBeam = false;
    double beamX = 0.0;
    double beamY = 0.0;
    double beamAngle = Limits::DefaultBeamAngle;
    if (beamPhrase >= 0) {
        bool ok = false;
        if (beamPhrase + 1 < lineCount) {
            const std::vector<double> nums = ExtractNumbers(lines[beamPhrase + 1]);
            if (nums.size() == 2 || nums.size() == 3) {
                ok = true;
                beamX = nums[0];
                beamY = nums[1];
                if (nums.size() == 3) beamAngle = nums[2];
            }
        }
        if (!ok) {
            errors.push_back(
                "После фразы «Луч:» нет значащей строки: сразу следующая строка должна содержать "
                "2 числа (координаты источника) или 3 (ещё и угол к стене в градусах).");
        } else if (!(beamAngle >= BeamAngle::Min && beamAngle <= BeamAngle::Max)) {
            errors.push_back(
                QString("Угол луча %1° вне допустимого диапазона: от %2° до %3° включительно.")
                    .arg(QString::number(beamAngle, 'f', BeamAngle::Decimals))
                    .arg(QString::number(BeamAngle::Min, 'f', BeamAngle::Decimals))
                    .arg(QString::number(BeamAngle::Max, 'f', BeamAngle::Decimals)));
        } else {
            haveBeam = true;
        }
    }

    if (!errors.empty()) {
        for (const QString &e : errors) Notify(NoteLevel::Error, e);
        return LoadResult::Failed;
    }

    // ---------- Этап 2: значения (тоже все сразу) ----------
    const int n = static_cast<int>(room.size());
    std::vector<Points> P;
    for (const RoomLine &r : room) P.push_back(Points(r.x, r.y));

    bool bad = false;

    for (int i = 0; i < n; ++i) {
        const double x = room[i].x;
        const double y = room[i].y;
        if (!(x >= Limits::MinX && x <= Limits::MaxX && y >= Limits::MinY && y <= Limits::MaxY)) {
            Notify(NoteLevel::Error,
                   QString("Точка %1 %2 вне рабочей области (допустимо: x от %3 до %4, y от %5 до %6).")
                       .arg(i + 1)
                       .arg(PointStr(P[i]))
                       .arg(Fmt(Limits::MinX)).arg(Fmt(Limits::MaxX))
                       .arg(Fmt(Limits::MinY)).arg(Fmt(Limits::MaxY)));
            bad = true;
        }
    }

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (P[i].Dist(P[j]) < Limits::MinPointDist) {
                Notify(NoteLevel::Error,
                       QString("Точки %1 и %2 слишком близко друг к другу (ближе %3 пикселей).")
                           .arg(i + 1).arg(j + 1)
                           .arg(static_cast<int>(Limits::MinPointDist)));
                bad = true;
            }
        }
    }

    for (int i = 0; i < n; ++i) {
        if (room[i].type == MirrorKind::Plane) continue;

        const double half = P[i].Dist(P[(i + 1) % n]) / 2.0;
        if (!std::isfinite(half)) continue;   // причина уже названа выше (вне области)

        if (!(room[i].radius >= half)) {
            Notify(NoteLevel::Error,
                   QString("Стена %1: радиус %2 меньше половины её длины (%3) - такое зеркало невозможно.")
                       .arg(i + 1)
                       .arg(FmtPrecise(room[i].radius))
                       .arg(FmtPrecise(half)));
            bad = true;
        } else if (room[i].radius > Limits::MaxMirrorRadius) {
            Notify(NoteLevel::Error,
                   QString("Стена %1: радиус %2 больше допустимого (%3).")
                       .arg(i + 1)
                       .arg(FmtPrecise(room[i].radius))
                       .arg(FmtPrecise(Limits::MaxMirrorRadius)));
            bad = true;
        }
    }

    if (bad) return LoadResult::Failed;

    // ---------- Этап 3: комната собирается отдельно, текущая не трогается ----------
    Polygon tmp;
    try {
        for (int i = 0; i < n; ++i) {
            bool flattened = false;
            tmp.sides_.push_back(
                MakeMirror(room[i].type, P[i], P[(i + 1) % n], room[i].radius, &flattened));
        }
    } catch (const std::exception &e) {
        Notify(NoteLevel::Error,
               QString("Не удалось построить комнату: %1").arg(QString::fromUtf8(e.what())));
        return LoadResult::Failed;
    }
    tmp.first = P[0];
    tmp.isExist_ = true;
    tmp.isCycled_ = true;

    std::vector<const Mirror *> w;
    for (const auto &side : tmp.sides_) w.push_back(side.get());

    int a = 0, b = 0;
    const WallCheck res = CheckWalls(w, true, {}, &a, &b);
    if (res == WallCheck::Cross) {
        Notify(NoteLevel::Error,
               QString("Комната некорректна: стены %1 и %2 пересекаются или касаются друг друга.")
                   .arg(a + 1).arg(b + 1));
        return LoadResult::Failed;
    }
    if (res == WallCheck::TooClose) {
        Notify(NoteLevel::Error,
               QString("Комната некорректна: стены %1 и %2 слишком близко друг к другу "
                       "(ближе толщины линии, %3 пикселей).")
                   .arg(a + 1).arg(b + 1).arg(Style::WallWidth));
        return LoadResult::Failed;
    }
    if (!tmp.Check()) {
        Notify(NoteLevel::Error,
               "Комната некорректна: стены пересекаются или касаются друг друга.");
        return LoadResult::Failed;
    }

    // Луч: проверяется так же, как клик по стене. Негодный луч не отменяет загрузку комнаты
    LoadResult result = LoadResult::RoomOnly;
    if (haveBeam) {
        const Points click(beamX, beamY);
        const BeamPlaceResult pr =
            tmp.PlaceBeam(click, Limits::BeamWallTolerance, Limits::MinBeamVertexDist);

        if (pr == BeamPlaceResult::Placed) {
            tmp.SetBeamAngle(beamAngle);
            result = LoadResult::RoomWithBeam;
        } else if (pr == BeamPlaceResult::NoWall) {
            Notify(NoteLevel::Error,
                   QString("Луч не поставлен: точка %1 не лежит ни на одной стене (допуск %2 пикселей). "
                           "Комната загружена без луча.")
                       .arg(PointStr(click))
                       .arg(static_cast<int>(Limits::BeamWallTolerance)));
        } else {
            Notify(NoteLevel::Error,
                   QString("Луч не поставлен: точка %1 ближе %2 пикселей к вершине. "
                           "Комната загружена без луча.")
                       .arg(PointStr(click))
                       .arg(static_cast<int>(Limits::MinBeamVertexDist)));
        }
    }

    // Всё в порядке: заменяем комнату
    prototype = std::move(tmp);

    Notify(NoteLevel::Info, QString("Из файла загружена комната: сторон %1.").arg(n));
    if (skippedLines > 0) {
        Notify(NoteLevel::Warning,
               QString("В файле больше %1 строк с углами: лишние строки (%2) пропущены, "
                       "в комнате не больше %1 сторон.")
                   .arg(Limits::MaxWalls)
                   .arg(skippedLines));
    }
    return result;
}
