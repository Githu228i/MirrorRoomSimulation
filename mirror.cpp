#include "mirror.h"

#include <QPainter>
#include <QPainterPath>

#include <cmath>


Mirror::Mirror(Points st, Points end)
    : start_(st),
    end_(end)
{
}


double Mirror::getRad() const
{
    return 0.0;
}


NotPlaneMirror::NotPlaneMirror(
    Points st,
    Points end,
    double rad
    )
    : Mirror(st, end),
    rad_(rad)
{
}


double NotPlaneMirror::getRad() const
{
    return rad_;
}


ConvexMirror::ConvexMirror(
    Points st,
    Points end,
    double rad
    )
    : NotPlaneMirror(st, end, rad)
{
}


ConcaveMirror::ConcaveMirror(
    Points st,
    Points end,
    double rad
    )
    : NotPlaneMirror(st, end, rad)
{
}


PlaneMirror::PlaneMirror(
    Points st,
    Points end
    )
    : Mirror(st, end)
{
}


Points Mirror::getStart() const
{
    return start_;
}


Points Mirror::getEnd() const
{
    return end_;
}


double Mirror::Dist() const
{
    return start_.Dist(end_);
}


/*
 * Получение центра окружности.
 *
 * leftSide = true:
 *     центр находится слева от направления
 *     start -> end.
 *
 * leftSide = false:
 *     центр находится справа.
 */
Points Mirror::CircleCenter(bool leftSide) const
{
    const double x1 = start_.getX();
    const double y1 = start_.getY();

    const double x2 = end_.getX();
    const double y2 = end_.getY();

    const double dx = x2 - x1;
    const double dy = y2 - y1;

    const double d = std::sqrt(
        dx * dx + dy * dy
        );

    const double radius = getRad();

    // Невозможно построить окружность.
    if (d == 0.0 || radius < d / 2.0)
    {
        return Points(
            (x1 + x2) / 2.0,
            (y1 + y2) / 2.0
            );
    }

    const double mx = (x1 + x2) / 2.0;
    const double my = (y1 + y2) / 2.0;

    const double h = std::sqrt(
        radius * radius
        - d * d / 4.0
        );

    /*
     * Левая нормаль к направлению start -> end:
     *
     * (-dy, dx)
     */
    double nx = -dy / d;
    double ny =  dx / d;

    if (!leftSide)
    {
        nx = -nx;
        ny = -ny;
    }

    return Points(
        mx + nx * h,
        my + ny * h
        );
}


bool Mirror::PointOnArc(
    const Points& p,
    const Points& center
    ) const
{
    const double radius = center.Dist(start_);
    const double distance = center.Dist(p);

    const double eps = 1e-6;

    return std::abs(distance - radius) < eps;
}


bool Mirror::Crossing(const Mirror& other) const
{
    Q_UNUSED(other);

    // Пока не реализовано.
    return false;
}


/*
 * Обычное плоское зеркало.
 */
void PlaneMirror::Draw(
    QPainter& painter,
    bool insideOnLeft
    ) const
{
    Q_UNUSED(insideOnLeft);

    painter.drawLine(
        start_.getX(),
        start_.getY(),
        end_.getX(),
        end_.getY()
        );
}


/*
 * Рисование круглой дуги.
 *
 * centerLeft:
 *   true  -> центр окружности слева от start -> end
 *   false -> справа.
 */
static void drawCircularArc(
    QPainter& painter,
    const Mirror& mirror,
    bool centerLeft
    )
{
    const double x1 = mirror.getStart().getX();
    const double y1 = mirror.getStart().getY();

    const double x2 = mirror.getEnd().getX();
    const double y2 = mirror.getEnd().getY();

    const double radius = mirror.getRad();

    const double dx = x2 - x1;
    const double dy = y2 - y1;

    const double distance =
        std::sqrt(dx * dx + dy * dy);

    // Совпадающие точки
    if (distance < 1e-9)
        return;

    // Радиус слишком маленький
    if (radius < distance / 2.0)
    {
        painter.drawLine(
            x1,
            y1,
            x2,
            y2
            );

        return;
    }

    /*
     * Середина хорды.
     */
    const double mx =
        (x1 + x2) / 2.0;

    const double my =
        (y1 + y2) / 2.0;

    /*
     * Единичная нормаль.
     *
     * (-dy, dx) — слева
     * ( dy,-dx) — справа
     */
    double nx = -dy / distance;
    double ny =  dx / distance;

    if (!centerLeft)
    {
        nx = -nx;
        ny = -ny;
    }

    /*
     * Расстояние от середины хорды
     * до центра окружности.
     */
    const double halfChord =
        distance / 2.0;

    const double h =
        std::sqrt(
            radius * radius
            - halfChord * halfChord
            );

    /*
     * Центр окружности.
     */
    const double cx =
        mx + nx * h;

    const double cy =
        my + ny * h;

    /*
     * Угол начала и конца.
     */
    const double pi =
        3.14159265358979323846;

    const double startAngle =
        std::atan2(
            y1 - cy,
            x1 - cx
            );

    const double endAngle =
        std::atan2(
            y2 - cy,
            x2 - cx
            );

    /*
     * Угол дуги.
     */
    double delta =
        endAngle - startAngle;

    /*
     * Нормализуем в диапазон
     * [-pi; pi].
     */
    while (delta > pi)
        delta -= 2.0 * pi;

    while (delta < -pi)
        delta += 2.0 * pi;

    /*
     * Qt:
     *
     * положительный угол —
     * против часовой стрелки.
     *
     * Поэтому направление зависит
     * от выбранной стороны.
     */
    if (centerLeft)
    {
        if (delta > 0)
            delta -= 2.0 * pi;
    }
    else
    {
        if (delta < 0)
            delta += 2.0 * pi;
    }

    /*
     * Если из-за направления получилась
     * длинная дуга — выбираем короткую.
     */
    if (delta > pi)
        delta = pi;

    if (delta < -pi)
        delta = -pi;

    /*
     * QRectF окружности.
     */
    QRectF rect(
        cx - radius,
        cy - radius,
        2.0 * radius,
        2.0 * radius
        );

    /*
     * drawArc рисует ТОЛЬКО дугу окружности,
     * а не сектор и не область внутри неё.
     *
     * Угол Qt задаётся в 1/16 градуса.
     */
    const int start =
        qRound(
            -startAngle
            * 180.0
            / pi
            * 16.0
            );

    const int span =
        qRound(
            -delta
            * 180.0
            / pi
            * 16.0
            );

    painter.drawArc(
        rect,
        start,
        span
        );
}


/*
 * ВЫПУКЛОЕ зеркало.
 *
 * Если внутренняя сторона полигона находится слева
 * от направления start -> end, центр окружности
 * должен находиться справа.
 *
 * Если внутренняя сторона справа,
 * центр должен быть слева.
 */
void ConvexMirror::Draw(
    QPainter& painter,
    bool insideOnLeft
    ) const
{
    /*
     * Для выпуклого зеркала центр находится
     * внутри полигона.
     */
    const bool centerLeft = insideOnLeft;

    drawCircularArc(
        painter,
        *this,
        centerLeft
        );
}


/*
 * ВОГНУТОЕ зеркало.
 *
 * Центр окружности находится снаружи полигона,
 * поэтому дуга выгибается внутрь комнаты.
 */
void ConcaveMirror::Draw(
    QPainter& painter,
    bool insideOnLeft
    ) const
{
    /*
     * Для вогнутого зеркала центр находится
     * снаружи полигона.
     */
    const bool centerLeft = !insideOnLeft;

    drawCircularArc(
        painter,
        *this,
        centerLeft
        );
}
