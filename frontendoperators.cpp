#include "frontendoperators.h"
#include <QWidget>

FrontEndOperators::FrontEndOperators() {}

void FrontEndOperators::CreatePoint(double x, double y) {
    if (prototype.sides_.size() < 9) {
        if (prototype.isExist()){
            if (prototype.sides_.size() == 0) {
                prototype.sides_.push_back(Mirror(prototype.first, Points(x, y)));
            } else {
                prototype.sides_.push_back(Mirror(prototype.sides_[prototype.sides_.size() - 1].getEnd(), Points(x, y)));
            }
        } else {
            prototype.isExist_ = true;
            prototype.first = Points(x, y);
        }
    }
    //update();
}
