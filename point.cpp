
struct Point {
    double x = 0, y = 0;
    Point(double e1, double e2): x(e1), y(e2) {}
};

struct Laser: public Point {
    double dig = 0;
    int pow = 100;
    Laser(double e1, double e2, double Dg): Point(e1, e2), dig(Dg) {}
};

int main() {
    return 0;
}
