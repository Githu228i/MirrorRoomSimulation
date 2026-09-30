#include "polygon.h"

Polygon::Polygon(const std::vector<Mirror>& sides, Beam beam) {
    sides_ = sides;
    beam_ = beam;
}

void Polygon::Write() {
    std::ofstream log("points.txt");
    if (log.is_open()) {
        log << std::endl;
        log << "-------------------------------" << std::endl;
        log << "Vertices of the polygon:" << std::endl;
        for (int i = 0; i < Size(); ++i) {
            log << sides_[i].getStart().Cords();
        }
        log << "\nBeam: " << beam_.Cords();
    } else {
        std::cerr << "The log cannot be opened." << std::endl;
        return;
    }
    log.close();
}

void Polygon::Push(const Mirror& other) {
    sides_.push_back(other);
    Write();
}

void Polygon::Del(int idx) {
    std::vector<Mirror> new_sides;
    for (int i = 0; i < Size(); ++i) {
        if (i == idx) continue;
        new_sides.push_back(sides_[i]);
    }
    sides_ = new_sides;
    Write();
}

int Polygon::Size() {
    return sides_.size();
}

bool Polygon::Check() {
    for (int i = 0; i < Size(); ++i) {
        for (int j = 0; j < Size(); ++j) {
            if (i == j) continue;
            
            if (sides_[i].Crossing(sides_[j])) return false;
        }
    }
    return true;
}


