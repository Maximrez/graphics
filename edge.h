#pragma once

#include "point.h"

class Edge {
public:
    Point<int> a;
    Point<int> b;
    Point<int> n;

    Edge() = default;

    Edge(const Point<int> &a, const Point<int> &b) : a(a), b(b) {
        n = {b.y - a.y, a.x - b.x, 0};
    }

    Edge(const Point<int> &a, const Point<int> &b, const Point<int> &n) : a(a), b(b), n(n) {}

    void draw(Magick::Image &img, const Magick::Color &color) const {
        draw_line(a.x, a.y, b.x, b.y, img, color);
    }

    Point<int> get_center() const {
        return (a + b) / 2;
    }

    Point<int> dir() const {
        return b - a;
    }
};

inline bool is_box_intersects(int a, int b, int c, int d) {
    if (a > b) swap(a, b);
    if (c > d) swap(c, d);
    return max(a, c) <= min(b, d);
}

enum PlaceType {
    PARALLEL,
    COLLINEAR,
    CROSS,
};

pair<bool, PlaceType> simple_intersection(const Point<int> &a, const Point<int> &b, const Point<int> &c, const Point<int> &d) {
    int ab_cd = -vec_area(b - a, d - c);
    if (ab_cd == 0) {
        if (vec_area(d - c, c - a) != 0)
            return {false, PARALLEL};
        return {is_box_intersects(a.x, b.x, c.x, d.x) && is_box_intersects(a.y, b.y, c.y, d.y), COLLINEAR};
    }

    double t1 = static_cast<double>(vec_area(d - c, c - a)) / ab_cd;
    double t2 = static_cast<double>(vec_area(b - a, c - a)) / ab_cd;

    return {0 <= t1 && t1 <= 1 && 0 <= t2 && t2 <= 1, CROSS};
}

pair<bool, PlaceType> simple_intersection(const Edge &first, const Edge &second) {
    return simple_intersection(first.a, first.b, second.a, second.b);
}

struct Intersection {
    double t;
    PlaceType place_type;
};

Intersection intersection_point(const Point<int> &a, const Point<int> &b, const Point<int> &c, const Point<int> &d) {
    int ab_cd = -vec_area(b - a, d - c);
    if (ab_cd == 0) {
        if (vec_area(d - c, c - a) != 0)
            return {0, PARALLEL};
        if (a.x == c.x) {
            int from_1 = min(a.y, b.y);
            int to_1 = max(a.y, b.y);
            int from_2 = min(c.y, d.y);
            int to_2 = max(c.y, d.y);
            return {double(from_2 - from_1) / (to_2 - to_1), COLLINEAR};
        } else {
            int from_1 = min(a.x, b.x);
            int to_1 = max(a.x, b.x);
            int from_2 = min(c.x, d.x);
            int to_2 = max(c.x, d.x);
            return {double(from_2 - from_1) / (to_2 - to_1), COLLINEAR};
        }
    }

    double t = static_cast<double>(vec_area(d - c, c - a)) / ab_cd;

    return {t, CROSS};
}

bool is_inside_segment(const Edge &edge, const Point<int> &v) {
    return (edge.a - v) * (edge.b - v) < 0;
}
