//
// Created by adanf on 15/03/2024.
//

#ifndef DRONECONTROLSYSTEM_UI_H
#define DRONECONTROLSYSTEM_UI_H
#include "MovingPoint.h"
#include "../globals.h"
#include "Drone_ui.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <sw/redis++/redis++.h>
#include <spdlog/spdlog.h>

using namespace sw::redis;

namespace ui {
    class UI {
    public:
        UI(Redis&);
        ~UI();

        int Run(std::reference_wrapper<drone_data, 300>);
    private:
        Redis& redis;
    };
}

#endif //DRONECONTROLSYSTEM_UI_H
