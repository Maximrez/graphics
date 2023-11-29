#pragma once

#include "polygon.h"

struct Face {
    array<Point<int>, 4> points;
    Point<int> center;
    Point<int> n;

    Face() = default;

    Face(const array<Point<int>, 4> &points,
         const Point<int> &center,
         const Point<int> &n) : points(points), center(center), n(n) {}

    void draw_bounds(Magick::Image &img, const Magick::Color &color) const {
        draw_line(points[0], points[1], img, color);
        draw_line(points[1], points[2], img, color);
        draw_line(points[2], points[3], img, color);
        draw_line(points[3], points[0], img, color);
    }
};

class Cube {
    Point<int> center;
public:
    array<Face, 6> faces;

    Cube(const Point<int> &p_min, int a, int b, int h) {
        array<Point<int>, 4> low, high;
        low[0] = p_min;
        low[1] = {p_min.x, p_min.y + b, p_min.z};
        low[2] = {p_min.x + a, p_min.y + b, p_min.z};
        low[3] = {p_min.x + a, p_min.y, p_min.z};

        for (size_t i = 0; i < low.size(); i++)
            high[i] = {low[i].x, low[i].y, low[i].z + h};

        faces[0] = Face(low, {}, {0, 0, 1});
        faces[1] = Face(high, {}, {0, 0, -1});

        array<Point<int>, 4> left = {low[0], low[1], high[1], high[0]};
        array<Point<int>, 4> up = {low[1], low[2], high[2], high[1]};
        array<Point<int>, 4> right = {low[2], low[3], high[3], high[2]};
        array<Point<int>, 4> down = {low[3], low[0], high[0], high[3]};

        faces[2] = Face(left, {}, {1, 0, 0});
        faces[3] = Face(up, {}, {0, -1, 0});
        faces[4] = Face(right, {}, {-1, 0, 0});
        faces[5] = Face(down, {}, {0, 1, 0});

        center = {p_min.x + a / 2, p_min.y + b / 2, p_min.z + h / 2};
        for (auto &face: faces) {
            Point<int> face_center(0, 0, 0);
            for (auto &v: face.points)
                face_center += v;
            face.center = face_center / 4;
        }

        fix_normals();
    }

    Point<int> get_center() const {
        return center;
    }

    void rotate(double alpha, double betta, double gamma, const Point<int> &r_center = {0, 0, 0}) {
        for (auto &face: faces) {
            for (auto &v: face.points) {
                v.rotate(alpha, betta, gamma, r_center);
            }
            face.center.rotate(alpha, betta, gamma, r_center);
        }
        center.rotate(alpha, betta, gamma, r_center);

        fix_normals();
    }

    void fix_normals() {
        for (auto &face: faces) {
            face.n = center - face.center;
        }
    }

    // Удаления невидимых ребер "проволочной" модели параллелепипеда.
    void draw(Magick::Image &img, const Magick::Color &color) const {
        for (auto &face: faces) {
            if (face.n.z < 0)
                face.draw_bounds(img, color);
        }
    }

    // Построение параллельной проекции повернутого параллелепипеда на плоскость Z = n.
    void draw_bounds(Magick::Image &img, const Magick::Color &color) const {
        for (auto &face: faces)
            face.draw_bounds(img, color);
    }

    // Построение одноточечной перспективной проекции повернутого параллелепипеда. Центр проекции находится в точке [0, 0, 1/r].
    void draw_one_point_projection(double r, Magick::Image &img, const Magick::Color &color) const {
        Point<int> center_projection = one_point_transform(center, r);
        for (auto &face: faces) {
            Point<int> face_center = one_point_transform(face.center, r);
            array<Point<int>, 4> points;
            for (size_t i = 0; i < face.points.size(); i++)
                points[i] = one_point_transform(face.points[i], r);

            Point<int> n = cross(points[1] - points[0], points[2] - points[1]);
            if (n * (center_projection - face_center) < 0)
                n = -n;
            if (n.z < 0)
                continue;

            draw_line(points[0], points[1], img, color);
            draw_line(points[1], points[2], img, color);
            draw_line(points[2], points[3], img, color);
            draw_line(points[3], points[0], img, color);
        }
    }

    // Построение одноточечной перспективной проекции повернутого параллелепипеда. Центр проекции находится в точке [1/p, 1/q, 0].
    void draw_two_point_projection(double p, double q, Magick::Image &img, const Magick::Color &color) const {
        Point<int> center_projection = two_point_transform(center, p, q);
        for (auto &face: faces) {
            Point<int> face_center = two_point_transform(face.center, p, q);
            array<Point<int>, 4> points;
            for (size_t i = 0; i < face.points.size(); i++)
                points[i] = two_point_transform(face.points[i], p, q);

            Point<int> n = cross(points[1] - points[0], points[2] - points[1]);
            if (n * (center_projection - face_center) < 0)
                n = -n;
            if (n.z < 0)
                continue;

            draw_line(points[0], points[1], img, color);
            draw_line(points[1], points[2], img, color);
            draw_line(points[2], points[3], img, color);
            draw_line(points[3], points[0], img, color);
        }
    }

private:

    Point<int> one_point_transform(const Point<int> &point, double r) const {
        return point.multiply(1.0 / (1 + r * point.z));
    }

    Point<int> two_point_transform(const Point<int> &point, double p, double q) const {
        return point.multiply(1.0 / (1 + p * point.x + q * point.y));
    }
};