#ifndef FRONTENDOPERATORS_H
#define FRONTENDOPERATORS_H

#include "points.h"
#include "mirror.h"
#include "polygon.h"
#include "changepointdialog.h"

#include <vector>

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
    void NextReflection();   // следующая точка отражения
    void ResetTrace();       // начать путь заново (история = только старт)

    friend class MainWindow;

private:
    Polygon prototype;
    Beam tempBeam_;
    std::vector<Points> history_;   // стартовая точка + точки отражений
};

#endif // FRONTENDOPERATORS_H
