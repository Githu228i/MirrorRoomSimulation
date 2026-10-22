#ifndef POLYGON_H
#define POLYGON_H

#include <vector>
#include <memory>
#include <fstream>
#include <iostream>
#include <utility>
#include "mirror.h"   // Mirror (с isFlat, CircleCenter, PointOnArc, Intersections...) и Beam
#include "points.h"

class FrontEndOperators;
class MainWindow;

enum class BeamPlaceResult { Placed, NoWall, TooClose };

class Polygon
{
public:
    Polygon();

    friend class MainWindow;
    friend class FrontEndOperators;

    // --- функции друга ---
    bool isExist() const;
    bool isCycled() const;

    // --- мои функции ---
    int  Size() const;

    // Контур замкнут и непрерывен, стороны не пересекаются и не касаются
    // нигде, кроме общих вершин соседних сторон.
    bool Check() const;


    // Ставит источник луча на ближайшую стену (не дальше maxDist от клика),
    // но не ближе minVertexDist к вершинам.
    BeamPlaceResult PlaceBeam(const Points& click, double maxDist, double minVertexDist);
    void SetBeamAngle(double wallAngleDeg);

    std::pair<Beam, Points> TraceBeamIter(const Beam& initial) const;

private:
    std::vector<std::unique_ptr<Mirror>> sides_;
    Points first;
    bool isExist_ = false;
    bool isCycled_ = false;
    Beam beam_;
    bool isBeamSet_ = false;
    int  beamWall_  = -1;   // индекс стены, на которой стоит источник
    double beamWallAngle_ = 90.0;   // угол к стене (градусы), для окна изменения угла
};

#endif // POLYGON_H
