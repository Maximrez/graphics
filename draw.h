#pragma once

#include <iostream>
#include <cmath>
#include <Magick++.h>
#include "point.h"

using namespace std;

void draw_line(int x1, int y1, int x2, int y2, Magick::Image &img, const Magick::Color &color) {
    if (x1 > x2) {
        swap(x1, x2);
        swap(y1, y2);
    }
    const int delta_x = abs(x2 - x1), delta_y = abs(y2 - y1);
    const int step_x = x1 < x2 ? 1 : -1, step_y = y1 < y2 ? 1 : -1;
    int error = delta_x - delta_y;
    img.pixelColor(x2, y2, color);
    while (x1 != x2 || y1 != y2) {
        img.pixelColor(x1, y1, color);
        int error2 = error * 2;
        if (error2 > -delta_y) {
            error -= delta_y;
            x1 += step_x;
        }
        if (error2 < delta_x) {
            error += delta_x;
            y1 += step_y;
        }
    }
}

void draw_line(const Point<int> &from, const Point<int> &to, Magick::Image &img, const Magick::Color &color) {
    draw_line(from.x, from.y, to.x, to.y, img, color);
}

void draw_bezier_curve_3(const vector<Point<int>> &init_points, Magick::Image &img, const Magick::Color &color) {
    if (init_points.size() != 4)
        throw runtime_error("Expected 4 points");

    size_t n = init_points.size();
    vector<int> coeffs = {1, 3, 3, 1};
    vector<Point<double>> curve_points(n);
    for (size_t i = 0; i < n; ++i) {
        curve_points[i] = to_double_point(init_points[i]);
    }
    Point<int> last = init_points[0];
    for (double t = 0.0; t <= 1.0; t += 0.01) {
        Point<double> p = {0.0, 0.0};
        for (size_t i = 0; i < n; i++) {
            p += curve_points[i] * (coeffs[i] * pow((1 - t), n - i - 1) * pow(t, i));
        }


        Point<int> cur = to_int_point(p);
        if ((cur - last).mod2() > 3) {
            draw_line(last.x, last.y, cur.x, cur.y, img, color);
            last = cur;
        }
    }
    draw_line(last, init_points.back(), img, color);
}

void draw_composite_bezier_curve_3(const vector<Point<int>> &init_points, Magick::Image &img, const Magick::Color &color) {
    for (int i = 0; i < init_points.size() - 1; i += 3) {
        if (init_points.size() <= i + 3)
            throw runtime_error("Wrong number of init_points");

        draw_bezier_curve_3({init_points[i], init_points[i + 1], init_points[i + 2], init_points[i + 3]}, img, color);
    }
}
