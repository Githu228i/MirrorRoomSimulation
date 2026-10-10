#include "frontendoperators.h"
#include <QWidget>
#include <memory>

FrontEndOperators::FrontEndOperators() {}

void FrontEndOperators::CreatePoint(double x, double y) {
    if (prototype.sides_.size() < 8) {
        if (prototype.isExist()){
            if (prototype.sides_.size() == 0) {
                prototype.sides_.push_back(std::make_unique<PlaneMirror>(prototype.first, Points(x, y)));
            } else {
                prototype.sides_.push_back(std::make_unique<PlaneMirror>(prototype.sides_[prototype.sides_.size() - 1]->getEnd(), Points(x, y)));
            }
        } else {
            prototype.isExist_ = true;
            prototype.first = Points(x, y);
        }
    }
    //update();
}

void FrontEndOperators::ConnectPoints(int num)
{
    Points start = prototype.sides_[num]->getStart();
    Points end = prototype.sides_.back()->getEnd();

    prototype.sides_.erase(
        prototype.sides_.begin(),
        prototype.sides_.begin() + num
        );
    prototype.first = start;

    prototype.sides_.push_back(std::make_unique<PlaneMirror>(end, start));
    prototype.isCycled_ = true;
}

void FrontEndOperators::ChangeMirror(
    int num,
    const ChangePointData &data)
{
    Points newPoint(data.x, data.y);

    // Последняя точка незамкнутого полигона
    if (!prototype.isCycled_
        && num == static_cast<int>(prototype.sides_.size()))
    {
        Points start =
            prototype.sides_.back()->getStart();

        prototype.sides_.back() =
            std::make_unique<PlaneMirror>(
                start,
                newPoint
                );

        return;
    }

    // Первая точка замкнутого полигона
    // Она одновременно является:
    // - началом первого зеркала
    // - концом последнего зеркала
    if (prototype.isCycled_ && num == 0)
    {
        prototype.first = newPoint;

        // Сохраняем конец первого зеркала
        Points firstEnd =
            prototype.sides_[0]->getEnd();

        // Сохраняем тип первого зеркала
        if (dynamic_cast<ConcaveMirror*>(
                prototype.sides_[0].get()))
        {
            double radius =
                prototype.sides_[0]->getRad();

            prototype.sides_[0] =
                std::make_unique<ConcaveMirror>(
                    newPoint,
                    firstEnd,
                    radius
                    );
        }
        else if (dynamic_cast<ConvexMirror*>(
                     prototype.sides_[0].get()))
        {
            double radius =
                prototype.sides_[0]->getRad();

            prototype.sides_[0] =
                std::make_unique<ConvexMirror>(
                    newPoint,
                    firstEnd,
                    radius
                    );
        }
        else
        {
            prototype.sides_[0] =
                std::make_unique<PlaneMirror>(
                    newPoint,
                    firstEnd
                    );
        }

        // Сохраняем начало последнего зеркала
        Points lastStart =
            prototype.sides_.back()->getStart();

        // Сохраняем тип последнего зеркала
        if (dynamic_cast<ConcaveMirror*>(
                prototype.sides_.back().get()))
        {
            double radius =
                prototype.sides_.back()->getRad();

            prototype.sides_.back() =
                std::make_unique<ConcaveMirror>(
                    lastStart,
                    newPoint,
                    radius
                    );
        }
        else if (dynamic_cast<ConvexMirror*>(
                     prototype.sides_.back().get()))
        {
            double radius =
                prototype.sides_.back()->getRad();

            prototype.sides_.back() =
                std::make_unique<ConvexMirror>(
                    lastStart,
                    newPoint,
                    radius
                    );
        }
        else
        {
            prototype.sides_.back() =
                std::make_unique<PlaneMirror>(
                    lastStart,
                    newPoint
                    );
        }

        return;
    }

    // Обычная точка
    Points end =
        prototype.sides_[num]->getEnd();

    // Изменяем конец предыдущего зеркала
    if (num > 0)
    {
        Points previousStart =
            prototype.sides_[num - 1]->getStart();

        // Сохраняем тип предыдущего зеркала
        if (dynamic_cast<ConcaveMirror*>(
                prototype.sides_[num - 1].get()))
        {
            double radius =
                prototype.sides_[num - 1]->getRad();

            prototype.sides_[num - 1] =
                std::make_unique<ConcaveMirror>(
                    previousStart,
                    newPoint,
                    radius
                    );
        }
        else if (dynamic_cast<ConvexMirror*>(
                     prototype.sides_[num - 1].get()))
        {
            double radius =
                prototype.sides_[num - 1]->getRad();

            prototype.sides_[num - 1] =
                std::make_unique<ConvexMirror>(
                    previousStart,
                    newPoint,
                    radius
                    );
        }
        else
        {
            prototype.sides_[num - 1] =
                std::make_unique<PlaneMirror>(
                    previousStart,
                    newPoint
                    );
        }
    }
    else
    {
        prototype.first = newPoint;
    }

    // Изменяем начало текущего зеркала
    // Тип зеркала берём из диалога
    switch (data.mirrorType)
    {
    case 0:
        prototype.sides_[num] =
            std::make_unique<PlaneMirror>(
                newPoint,
                end
                );
        break;

    case 1:
        prototype.sides_[num] =
            std::make_unique<ConcaveMirror>(
                newPoint,
                end,
                data.radius
                );
        break;

    case 2:
        prototype.sides_[num] =
            std::make_unique<ConvexMirror>(
                newPoint,
                end,
                data.radius
                );
        break;
    }
}
