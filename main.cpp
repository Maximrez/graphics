#define MAGICKCORE_QUANTUM_DEPTH 16
#define MAGICKCORE_HDRI_ENABLE 1

#include <iostream>
#include <cassert>
#include <vector>
#include "polygon.h"
#include "cube.h"
#include <Magick++.h>

using namespace std;

const int DEPTH = 65535;
const Magick::Color Red(DEPTH, 0, 0);
const Magick::Color Green(0.4 * DEPTH, 0.8 * DEPTH, 0.4 * DEPTH);
const Magick::Color Blue(0, 0, DEPTH);
const Magick::Color Black(0, 0, 0);
const Magick::Color Orange(DEPTH, 0.3 * DEPTH, 0);

void save_img(Magick::Image &img, const string &filename) {
    img.flip();
    img.magick("png");
    img.write("../images/" + filename);
}

// Вычерчивания отрезков прямых линий толщиной в 1 пиксел
void test_draw_line() {
    Magick::Image img("100x100", "white");
    draw_line(10, 20, 50, 20, img, Black);
    draw_line(20, 40, 20, 60, img, Green);
    save_img(img, "line1.png");

    Magick::Image img2("100x100", "white");
    draw_line(0, 0, 30, 80, img2, Black);
    draw_line(30, 80, 0, 0, img2, Green);
    save_img(img2, "line2.png");

    Magick::Image img3("100x100", "white");
    draw_line(10, 80, 80, 10, img3, Black);
    save_img(img3, "line3.png");
}

// Определения типа полигона: простой или сложный (т.е. с самопересечениями), выпуклый или невыпуклый
void test_polygon_type() {
    Polygon triangle({{10, 10},
                      {20, 30},
                      {30, 20}});
    assert(triangle.is_simple());
    assert(triangle.is_convex());

    Polygon pol1({{10, 10},
                  {10, 20},
                  {20, 20},
                  {15, 12}});
    assert(pol1.is_simple());
    assert(pol1.is_convex());

    Polygon pol2({{10, 10},
                  {10, 20},
                  {20, 20},
                  {13, 16}});
    assert(pol2.is_simple());
    assert(!pol2.is_convex());

    Polygon pol3({{10, 10},
                  {20, 10},
                  {10, 20},
                  {20, 20}});
    assert(!pol3.is_simple());
    assert(!pol3.is_convex());

    Magick::Image img("50x50", "white");
    pol3.draw_bounds(img, Black);
    save_img(img, "test_polygon_type.png");
}

// Заполнения полигона, используя правила even-odd и non-zero-winding определения принадлежности пикселя полигону.
void test_stars() {
    vector<Point<int>> points = {{150, 200},
                                 {460, 350},
                                 {90,  350},
                                 {400, 200},
                                 {250, 460}};
    Magick::Image img("1000x800", "white");
    Polygon star1(points);
    star1.move({60, -150});
    star1.fill_polygon(Polygon::FillingMethod::NonZeroWinding, img, Orange);
    star1.draw_bounds(img, Black);

    Polygon star2(points);
    star2.move({500, 300});
    star2.fill_polygon(Polygon::FillingMethod::EvenOddRule, img, Orange);
    star2.draw_bounds(img, Black);

    assert(!star1.is_simple());
    assert(!star1.is_convex());

    save_img(img, "stars.png");
}

// Построения кривых Безье третьего порядка
void test_bezier() {
    Magick::Image img("300x300", "white");
    draw_bezier_curve_3({{220, 200},
                         {50,  300},
                         {50,  70},
                         {210, 280}}, img, Black);
    save_img(img, "bezier_line.png");
}

// Напишите программу, которая строит составную кубическую кривую Безье
void test_composite_bezier() {
    Magick::Image img("500x500", "white");
    draw_composite_bezier_curve_3({{100, 200},
                                   {150, 250},
                                   {200, 300},
                                   {250, 250},
                                   {300, 200},
                                   {350, 300},
                                   {400, 200}}, img, Black);
    save_img(img, "bezier_composite_line.png");
}

// Отсечения отрезков прямых выпуклым полигоном
void test_draw_clip() {
    auto clipping = [](Magick::Image &img, const vector<Point<int>> &points) {
        Polygon pol(points);
        assert(pol.is_simple());
        assert(pol.is_convex());
        pol.draw_bounds(img, Black);

        vector<Edge> lines;
        lines.emplace_back(Point{100, 200}, Point{350, 400});
        lines.emplace_back(Point{100, 350}, Point{400, 20});
        lines.emplace_back(Point{200, 250}, Point{300, 300});
        lines.emplace_back(Point{400, 200}, Point{450, 250});
        lines.emplace_back(Point{310, 310}, Point{400, 340});

        for (auto &line: lines) {
            line.draw(img, Green);
            Edge clip_line = cyrus_beck_clip_line(line, pol);
            if (clip_line.a != clip_line.b)
                clip_line.draw(img, Blue);
        }
    };

    vector<Point<int>> points = {Point{100, 100}, Point{250, 350}, Point{400, 400}, Point{380, 320}, Point{250, 150}};
    Magick::Image img("500x500", "white");
    clipping(img, points);
    save_img(img, "clip_line.png");

    std::reverse(points.begin(), points.end());
    Magick::Image img2("500x500", "white");
    clipping(img2, points);
    save_img(img2, "clip_line_reverse.png");
}

void test_projection() {
    Magick::Image img("500x500", "white");

    Cube cube({200, 200, 100}, 100, 100, 100);
    cube.rotate(M_PI / 4, M_PI / 8, 0, cube.get_center());
    cube.draw_bounds(img, Black);
    cube.draw(img, Blue);
    save_img(img, "projection.png");
}

void test_two_point_projection() {
    Magick::Image img("500x500", "white");

    Cube cube({200, 200, 100}, 200, 400, 300);
    cube.rotate(0, M_PI / 8, M_PI / 4, cube.get_center());
    cube.draw_two_point_projection(0.001, 0.002, img, Black);

    save_img(img, "two_point_projection.png");
}

void draw_animation() {
    int a = 200, b = 200, h = 300;
    Cube cube({200, 200, 0}, a, b, h);
    cube.rotate(M_PI_4, 0, M_PI / 4, cube.get_center());

    int N = 50;
    vector<Magick::Image> frames1(N, Magick::Image("700x700", "white"));
    vector<Magick::Image> frames2(N, Magick::Image("700x700", "white"));
    auto center = cube.get_center();
    for (int i = 0; i < N; i++) {
        cube.rotate(0, 2 * M_PI / N, 0, center);
        cube.draw_one_point_projection(1e-3, frames1[i], Blue);
        cube.draw(frames2[i], Blue);
    }

    for (auto &frame: frames1) {
        frame.flip();
        frame.animationDelay(1);
    }
    for (auto &frame: frames2) {
        frame.flip();
        frame.animationDelay(1);
    }

    Magick::writeImages(frames1.begin(), frames1.end(), "../images/anim.gif");
    Magick::writeImages(frames2.begin(), frames2.end(), "../images/anim2.gif");
}

void test_weiler_atherton1() {
    Magick::Image img("500x500", "white");
    vector<Point<int>> points1 = {{50,  50},
                                  {100, 200},
                                  {200, 300},
                                  {300, 100}};
    vector<Point<int>> points2 = {{100, 400},
                                  {300, 300},
                                  {150, 100}};
    Polygon pol1(points1);
    Polygon pol2(points2);
    pol1.draw_bounds(img, Blue);
    pol2.draw_bounds(img, Green);

    Polygon res = weiler_atherton(pol1, pol2);
    res.draw_bounds(img, Red);

    std::reverse(points1.begin(), points1.end());
    std::reverse(points2.begin(), points2.end());
    Polygon res2 = weiler_atherton(Polygon(points1), Polygon(points2));
    res2.draw_bounds(img, Black);

    save_img(img, "weiler_atherton1.png");
}

void test_weiler_atherton2() {
    Magick::Image img("500x500", "white");
    vector<Point<int>> points1 = {{100, 200},
                                  {100, 450},
                                  {300, 450},
                                  {300, 200}};
    vector<Point<int>> points2 = {{150, 150},
                                  {150, 400},
                                  {400, 400},
                                  {250, 300},
                                  {400, 150}};
    Polygon pol1(points1);
    Polygon pol2(points2);
    pol1.draw_bounds(img, Blue);
    pol2.draw_bounds(img, Green);

    Polygon res = weiler_atherton(pol1, pol2);
    res.draw_bounds(img, Red);

    std::reverse(points1.begin(), points1.end());
    std::reverse(points2.begin(), points2.end());
    Polygon res2 = weiler_atherton(Polygon(points1), Polygon(points2));
    res2.draw_bounds(img, Black);

    save_img(img, "weiler_atherton2.png");
}

// тест на случай если полигоны не пересекаются
void test_weiler_atherton3() {
    Magick::Image img("500x500", "white");
    vector<Point<int>> points1 = {{50,  50},
                                  {100, 200},
                                  {200, 300}};
    vector<Point<int>> points2 = {{300, 400},
                                  {350, 300},
                                  {250, 300}};
    Polygon pol1(points1);
    Polygon pol2(points2);
    pol1.draw_bounds(img, Blue);
    pol2.draw_bounds(img, Green);

    Polygon res = weiler_atherton(pol1, pol2);
    res.draw_bounds(img, Red);

    std::reverse(points1.begin(), points1.end());
    std::reverse(points2.begin(), points2.end());
    Polygon res2 = weiler_atherton(Polygon(points1), Polygon(points2));
    res2.draw_bounds(img, Black);

    save_img(img, "weiler_atherton3.png");
}

int main() {
//    test_polygon_type();
//    test_draw_line();
//    test_stars();
//    test_bezier();
//    test_composite_bezier();
//    test_draw_clip();
    test_projection();
    test_two_point_projection();
//    draw_animation();
    test_weiler_atherton1();
    test_weiler_atherton2();
    test_weiler_atherton3();
    return 0;
}