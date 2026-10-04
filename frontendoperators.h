#ifndef FRONTENDOPERATORS_H
#define FRONTENDOPERATORS_H


#include "points.h"
#include "mirror.h"
#include "polygon.h"
// #include "mainwindow.h"

class FrontEndOperators
{
public:
    FrontEndOperators();
    void CreatePoint(double x, double y);
    void CreateMirror(Points fir, Points sec);
    friend class MainWindow;
private:
    Polygon prototype;
};

#endif // FRONTENDOPERATORS_H
