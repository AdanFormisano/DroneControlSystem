//
// Created by adanf on 15/03/2024.
//

#ifndef DRONECONTROLSYSTEM_MOVINGPOINT_H
#define DRONECONTROLSYSTEM_MOVINGPOINT_H
#include "../globals.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <sw/redis++/redis++.h>

using namespace sw::redis;

class MovingPoint : public Fl_Widget {
private:
    int size = 10;      // size of the point
    int X = 0, Y = 0;   // position of the point

public:
    Redis& redis;

    MovingPoint(Redis&, int, int, int, int);
    void draw() override;

};


#endif //DRONECONTROLSYSTEM_MOVINGPOINT_H
