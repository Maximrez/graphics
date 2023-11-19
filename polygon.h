#pragma once

#include "draw.h"
#include "edge.h"
#include <cmath>

class BBox {
public:
    int x_min, x_max;
    int y_min, y_max;

    BBox() : x_min(0), x_max(0), y_min(0), y_max(0) {}

    explicit BBox(const vector<Edge> &edges) {
        x_min = x_max = edges[0].a.x;
        y_min = y_max = edges[0].a.y;
        for (auto &edge: edges) {
            x_min = min(x_min, edge.a.x);
            x_max = max(x_max, edge.a.x);

            y_min = min(y_min, edge.a.y);
            y_max = max(y_max, edge.a.y);
        }
    }

    ~BBox() = default;
};

template<typename T>
int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

class Polygon {
private:
    vector<Edge> edges;
    BBox bbox;
public:
    explicit Polygon(vector<Point<int>> points) {
        // проверяем, что полигон ориентирован по часовой стрелке
        if (vec_area(points[1] - points[0], points[2] - points[0]) > 0)
            std::reverse(points.begin(), points.end());

        edges = vector<Edge>(points.size());
        for (size_t i = 0; i < points.size(); i++) {
            edges[i] = {points[i], points[(i + 1) % points.size()]};
        }

        bbox = BBox(edges);

        auto center = get_center();
        for (auto &edge: edges) {
            if (edge.n * (center - edge.get_center()) < 0)
                edge.n = -edge.n;
        }
    };

    void draw_bounds(Magick::Image &img, const Magick::Color &color) {
        for (auto &edge: edges)
            edge.draw(img, color);
    }

    bool is_simple() const {
        int n = edges.size();
        if (n <= 2)
            return false;

        // проверяем первую грань со всеми
        for (int i = 2; i < n - 1; ++i) {
            if (simple_intersection(edges[0], edges[i]).first)
                return false;
        }

        // проверяем остальные
        for (int i = 1; i < n - 2; ++i) {
            for (int j = i + 2; j < n; ++j) {
                if (simple_intersection(edges[i], edges[j]).first)
                    return false;
            }
        }

        return true;
    }

    bool is_convex() const {
        if (edges.size() <= 2 || !is_simple())
            return false;

        int sign = sgn(vec_area(edges[0].dir(), edges[1].dir()));
        for (size_t i = 1; i < edges.size(); i++) {
            if (sign * vec_area(edges[i].dir(), edges[(i + 1) % edges.size()].dir()) < 0)
                return false;
        }

        return true;
    }

    bool is_inside_even_odd_rule(const Point<int> &point) const {
        if (edges.size() <= 2)
            return false;

        Point<int> end(0, point.y - 10);
        Edge line = {point, end};
        int count = 0;
        for (auto &edge: edges) {
            auto check = simple_intersection(edge, line);
            if (check.first) {
                if (check.second == CROSS || is_inside_segment(edge, point))
                    count++;
            }
        }

        return count % 2 != 0;
    }

    bool is_inside_non_zero_winding(const Point<int> &point) const {
        if (edges.size() <= 2)
            return false;

        Point<int> end(0, point.y - 10);
        Point l = end - point;
        Edge line = {point, end};
        int winding = 0;
        for (auto &edge: edges) {
            auto res = simple_intersection(edge, line);
            if (res.first) {
                if (res.second == CROSS || is_inside_segment(edge, point))
                    winding += vec_area(edge.dir(), l) > 0 ? 1 : -1;
            }
        }

        return winding != 0;
    }

    enum FillingMethod {
        EvenOddRule,
        NonZeroWinding,
    };

    void fill_polygon(FillingMethod method, Magick::Image &img, const Magick::Color &color) const {
        if (edges.size() <= 2)
            return;

        switch (method) {
            case EvenOddRule:
                for (int i = bbox.x_min; i < bbox.x_max; i++) {
                    for (int j = bbox.y_min; j < bbox.y_max; j++) {
                        if (is_inside_even_odd_rule({i, j}))
                            img.pixelColor(i, j, color);
                    }
                }
                break;
            case NonZeroWinding:
                for (int i = bbox.x_min; i < bbox.x_max; i++) {
                    for (int j = bbox.y_min; j < bbox.y_max; j++) {
                        if (is_inside_non_zero_winding({i, j}))
                            img.pixelColor(i, j, color);
                    }
                }
                break;
        }
    }

    Point<int> get_center() const {
        Point<int> center;
        for (auto &edge: edges) {
            center += edge.a;
        }

        return center / edges.size();
    }

    void move(const Point<int> &shift) {
        for (auto &edge: edges) {
            edge.a += shift;
            edge.b += shift;
        }
        bbox.x_min += shift.x;
        bbox.x_max += shift.x;
        bbox.y_min += shift.y;
        bbox.y_max += shift.y;
    }

    vector<Edge> get_edges() const {
        return edges;
    }

    ~Polygon() = default;
};

Edge cyrus_beck_clip_line(const Edge &line, const Polygon &pol) {
    Point<int> l = line.dir();
    double t1 = 0, t2 = 1;
    for (auto &edge: pol.get_edges()) {
        Intersection info = intersection_point(line.a, line.b, edge.a, edge.b);
        if (info.place_type == PlaceType::PARALLEL)
            continue;
        if (l * edge.n > 0) {
            t1 = max(t1, info.t);
        } else {
            t2 = min(t2, info.t);
        }
    }

    if (t1 > t2)
        return Edge{line.a, line.a};

    Point<int> a = line.a + l.multiply(t1);
    Point<int> b = line.a + l.multiply(t2);

    return Edge{a, b};
}
