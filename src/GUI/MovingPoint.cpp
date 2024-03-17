//
// Created by adanf on 15/03/2024.
//

#include "MovingPoint.h"

void MovingPoint::draw() {
    // Set the color of the point
    fl_color(FL_RED);

    // Draw the point as a filled box
    fl_rectf(X, Y, size, size);

    // Get info from Redis
    std::pair<float, float> position;
    auto r = redis.get("drone:0:position");

    if (r) {
        position = std::make_pair(std::stof(r.value().substr(0, r.value().find(','))), std::stof(r.value().substr(r.value().find(',') + 1)));
        X = position.first;
        Y = position.second;
    }
}

MovingPoint::MovingPoint(Redis& redis, int x,int y,int w,int h) : Fl_Widget(x,y,w,h), redis(redis) {
    // Position the point at the center initially
    X = w / 2;
    Y = h / 2;
}