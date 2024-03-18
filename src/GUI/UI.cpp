//
// Created by adanf on 15/03/2024.
//

#include "UI.h"

namespace ui {
    UI::UI(Redis& redis) : redis(redis){
        spdlog::info("UI process starting");
    }

    UI::~UI() {
        spdlog::info("UI process ending");
    }

    int UI::Run() {
        // Main window
        Fl_Double_Window window(WINDOW_WIDTH, WINDOW_HEIGHT, "Drone Control System");
        DroneUI dui(0,0);

        window.add(dui);

//        window = new Fl_Window(1200,1200);
//        window->label("Drone Control System");

//        MovingPoint *point = new MovingPoint(redis, 0, 0, window->w(), window->h());
//        window->add(point);


        // window->callback(close_cb);
        window.end();

        // Get the main screen dimensions
        int screen_w = Fl::w();
        int screen_h = Fl::h();

        // Calculate the window position to center it on the main screen
        int window_x = (screen_w) / 2;
        int window_y = (screen_h - window.h()) / 2;

        // Set the window position
        window.position(window_x, window_y);

        window.show();

        Fl::run();
        Fl::first_window()->hide();

        return 0;
    }
}