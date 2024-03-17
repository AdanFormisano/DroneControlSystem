//
// Created by adanf on 16/03/2024.
//

#ifndef DRONECONTROLSYSTEM_DRONE_UI_H
#define DRONECONTROLSYSTEM_DRONE_UI_H
#include "../globals.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>

class DroneUI : public Fl_Widget {
public:
    DroneUI(int x, int y);

    void draw() override;
    void update();
    static void timer_cb(void *userdata);
};

#endif //DRONECONTROLSYSTEM_DRONE_UI_H
