#pragma once

#include "draw.h"
#include "edge.h"
#include <cmath>
#include <map>

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
    Polygon() = default;

    explicit Polygon(vector<Point<int>> points) {
        // проверяем, что полигон ориентирован по часовой стрелке
        int area = 0;
        for (int i = 1; i < points.size(); i++) {
            area += vec_area(points[i] - points[i - 1], points[(i + 1) % points.size()] - points[i - 1]);
        }
        if (area > 0)
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

    size_t size() const {
        return edges.size();
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
            t1 = max(t1, info.t1);
        } else {
            t2 = min(t2, info.t1);
        }
    }

    if (t1 > t2)
        return Edge{line.a, line.a};

    Point<int> a = line.a + l.multiply(t1);
    Point<int> b = line.a + l.multiply(t2);

    return Edge{a, b};
}

// Отсечение произвольного простого полигона по произвольному простому полигону, используя алгоритм Вейлера-Айзертона.
// При реализации можно сделать несколько упрощений:
// в случае, когда в результате отсечения образуется несколько полигонов, результатом может служить любой из этих полигонов;
// вершины исходных полигонов не должны лежать на ребрах друг друга.
Polygon weiler_atherton(const Polygon &orig, const Polygon &cutter) {
    vector<Edge> orig_edges = orig.get_edges();
    vector<Edge> cutter_edges = cutter.get_edges();
    vector<Point<int>> list1, list2;
    list1.reserve(1.5 * orig_edges.size());
    list2.reserve(1.5 * cutter_edges.size());

    map<Point<int>, size_t> m1, m2;
    map<pair<int, int>, Intersection> new_points; // храним точки пересечения для отрезков, чтобы избежать магии округления и два раза не считать

    for (size_t i = 0; i < orig_edges.size(); i++) {
        auto orig_edge = orig_edges[i];
        list1.push_back(orig_edge.a);

        vector<pair<double, Point<int>>> intersects;
        for (size_t j = 0; j < cutter_edges.size(); j++) {
            Intersection ans = intersection_point(orig_edge, cutter_edges[j]);
            double t1 = ans.t1, t2 = ans.t2;
            if (ans.place_type != PlaceType::CROSS || t1 <= 0 || t1 >= 1 || t2 <= 0 || t2 >= 1)
                continue;
            Point<int> point = orig_edge.a + (orig_edge.b - orig_edge.a).multiply(t1);
            intersects.emplace_back(t1, point);
            new_points[{i, j}] = ans;
        }

        if (intersects.empty())
            continue;
        sort(intersects.begin(), intersects.end());
        for (auto &[t, point]: intersects) {
            list1.push_back(point);
            m2[point] = list1.size() - 1;
        }
    }

    for (size_t j = 0; j < cutter_edges.size(); j++) {
        list2.push_back(cutter_edges[j].a);
        vector<pair<double, Point<int>>> intersects;
        for (size_t i = 0; i < orig_edges.size(); i++) {
            if (!new_points.contains({i, j}))
                continue;
            auto ans = new_points[{i, j}];
            Point<int> point = orig_edges[i].a + (orig_edges[i].b - orig_edges[i].a).multiply(ans.t1);
            intersects.emplace_back(ans.t2, point);
        }

        if (intersects.empty())
            continue;
        sort(intersects.begin(), intersects.end());
        for (auto &[t, point]: intersects) {
            list2.push_back(point);
            m1[point] = list2.size() - 1;
        }
    }

    size_t begin = list1.size() + 1; // нужно начать с точки которая находится внутри отсекателя
    for (auto &edge: orig_edges)
        if (cutter.is_inside_even_odd_rule(edge.a)) {
            begin = distance(list1.begin(), find(list1.begin(), list1.end(), edge.a));
            break;
        }

    // если все точки находятся снаружи
    if (begin == list1.size() + 1)
        return Polygon();

    size_t idx1 = (begin + 1) % list1.size();
    vector<Point<int>> points = {list1[begin]};
    while (idx1 != begin) {
        points.push_back(list1[idx1]);
        if (!m1.contains(list1[idx1])) {
            idx1 = (idx1 + 1) % list1.size();
            continue;
        }

        size_t idx2 = (m1[list1[idx1]] + 1) % list2.size();
        while (true) {
            points.push_back(list2[idx2]);
            if (!m2.contains(list2[idx2])) {
                idx2 = (idx2 + 1) % list2.size();
            } else {
                idx1 = (m2[list2[idx2]] + 1) % list1.size();
                break;
            }
        }
    }

    return Polygon(points);
}
