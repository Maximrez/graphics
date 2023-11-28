#pragma once

#include <iostream>
#include <cmath>

using namespace std;

template<typename T>
class Point {
public:
    T x = 0, y = 0, z = 0;

    Point(T _x = 0, T _y = 0) : x(_x), y(_y), z(0) {}

    Point(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}

    Point operator+(const Point &a) const {
        return Point(a.x + x, a.y + y, a.z + z);
    }

    Point operator+=(const Point &a) {
        x += a.x;
        y += a.y;
        z += a.z;
        return *this;
    }

    Point operator-(const Point &a) const {
        return Point(x - a.x, y - a.y, z - a.z);
    }

    T mod2() const {
        return x * x + y * y + z * z;
    }

    void rotate(double alpha, double betta, double gamma, const Point<int> &center = {0, 0, 0}) {
        double cos_a = cos(alpha), sin_a = sin(alpha);
        double cos_b = cos(betta), sin_b = sin(betta);
        double cos_g = cos(gamma), sin_g = sin(gamma);
        Point<int> p = *this - center;
        x = center.x + cos_b * cos_g * p.x - sin_g * cos_b * p.y + sin_b * p.z;
        y = center.y +
            (sin_a * sin_b * cos_g + sin_g * cos_a) * p.x +
            (-sin_a * sin_b * sin_g + cos_a * cos_g) * p.y -
            sin_a * cos_b * p.z;
        z = center.z +
            (sin_a * sin_g - sin_b * cos_a * cos_g) * p.x +
            (sin_a * cos_g + sin_b * sin_g * cos_a) * p.y +
            cos_a * cos_b * p.z;
    }

    template<typename C>
    Point operator*(const C &a) const {
        return Point(x * a, y * a, z * a);
    }

    Point<int> multiply(double c) const {
        return Point(int(round(c * x)), int(round(c * y)), int(round(c * z)));
    }

    template<typename C>
    Point operator/(const C &a) const {
        return Point(x / a, y / a, z / a);
    }

    T operator*(const Point &a) const {
        return a.x * x + a.y * y + a.z * z;
    }

    Point operator-() const {
        return Point(-x, -y, -z);
    }

    auto operator<=>(const Point &) const = default;

    ~Point() = default;
};

template<typename T>
inline T vec_area(const Point<T> &a, const Point<T> &b) {
    return a.x * b.y - a.y * b.x;
}

template<typename T>
inline std::ostream &operator<<(std::ostream &os, const Point<T> &a) {
    os << '{' << a.x << ", " << a.y << ", " << a.z << '}';
    return os;
}

Point<double> to_double_point(const Point<int> &a) {
    return Point<double>{(double) a.x, (double) a.y, (double) a.z};
}

Point<int> to_int_point(const Point<double> &a) {
    return Point<int>{int(round((a.x))), int(round((a.y))), int(round((a.z)))};
}

template<typename T>
Point<T> cross(const Point<T> &a, const Point<T> &b) {
    return Point(a.y * b.z - a.z * b.y, -a.x * b.z + a.z * b.x, a.x * b.y - a.y * b.x);
}