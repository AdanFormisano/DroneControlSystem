//
// Created by adanf on 16/03/2024.
//

#include "Drone_ui.h"

DroneUI::DroneUI(int x, int y) : Fl_Widget(x, y, WINDOW_WIDTH, WINDOW_HEIGHT){
    Fl::add_timeout(0.1, timer_cb, this);
}

void DroneUI::draw() {
    fl_color(FL_RED);
    fl_rectf(WINDOW_WIDTH/2,WINDOW_HEIGHT/2, 2, 2);
}

void DroneUI::update() {
    redraw();
    Fl::repeat_timeout(0.1, timer_cb, this);
}

void DroneUI::timer_cb(void *userdata) {
    static_cast<DroneUI*>(userdata)->update();
}

