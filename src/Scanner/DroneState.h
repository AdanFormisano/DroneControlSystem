#ifndef DRONESTATE_H
#define DRONESTATE_H

#include "Drone.h"
#include "spdlog/spdlog.h"

class Drone;

class DroneState
{
public:
    virtual void enter(Drone* drone) = 0;
    virtual void run(Drone* drone) = 0;
    virtual void exit(Drone* drone) = 0;
    virtual drone_state_enum getState() = 0;
    virtual ~DroneState() = default;
};

class Idle : public DroneState
{
public:
    void enter(Drone* drone) {};
    void run(Drone* drone) override;
    void exit(Drone* drone) {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::IDLE; }

private:
    Idle() {}
    Idle(const Idle& other);
    Idle& operator=(const Idle& other);
};

class ToStartingLine : public DroneState
{
public:
    void enter(Drone* drone) override;
    void run(Drone* drone) override;
    void exit(Drone* drone) override;
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::TO_STARTING_LINE; }
private:
    ToStartingLine() {}
    ToStartingLine(const ToStartingLine& other);
    ToStartingLine& operator=(const ToStartingLine& other);
};

class Ready : public DroneState
{
public:
    void enter(Drone* drone) override;
    void run(Drone* drone) override;
    void exit(Drone* drone) override {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::READY; }
private:
    Ready() {}
    Ready(const Ready& other);
    Ready& operator=(const Ready& other);
};

class Working : public DroneState
{
public:
    void enter(Drone* drone) override;
    void run(Drone* drone) override;
    void exit(Drone* drone) {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::WORKING; }
private:
    Working() {}
    Working(const Working& other);
    Working& operator=(const Working& other);
};

class ToBase : public DroneState
{
public:
    void enter(Drone* drone) override;
    void run(Drone* drone) override;
    void exit(Drone* drone) override;
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::TO_BASE; }
private:
    ToBase() {}
    ToBase(const ToBase& other);
    ToBase& operator=(const ToBase& other);
};

class Charging : public DroneState
{
public:
    void enter(Drone* drone) override;
    void run(Drone* drone) override;
    void exit(Drone* drone) {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::CHARGING; }
private:
    Charging() {}
    Charging(const Charging& other);
    Charging& operator=(const Charging& other);
};

class Dead : public DroneState
{
public:
    void enter(Drone* drone) {};
    void run(Drone* drone) override;
    void exit(Drone* drone) {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::DEAD; }
private:
    Dead() {}
    Dead(const Dead& other);
    Dead& operator=(const Dead& other);
};

class Disconnected : public DroneState
{
public:
    void enter(Drone* drone) {};
    void exit(Drone* drone) {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::DISCONNECTED; }
private:
    Disconnected() {}
    Disconnected(const Disconnected& other);
    Disconnected& operator=(const Disconnected& other);
};

class Reconnected : public DroneState
{
public:
    void enter(Drone* drone) {};
    void exit(Drone* drone) {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::RECONNECTED; }
private:
    Reconnected() {}
    Reconnected(const Reconnected& other);
    Reconnected& operator=(const Reconnected& other);
};

class HighConsumption : public DroneState
{
public:
    void enter(Drone* drone) {};
    void exit(Drone* drone) {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::NONE; }
private:
    HighConsumption() {}
    HighConsumption(const HighConsumption& other);
    HighConsumption& operator=(const HighConsumption& other);
};

inline DroneState& getDroneState(drone_state_enum state) {
    switch (state) {
    case drone_state_enum::TO_STARTING_LINE:
        return ToStartingLine::getInstance();
    case drone_state_enum::READY:
        return Ready::getInstance();
    case drone_state_enum::WORKING:
        return Working::getInstance();
    case drone_state_enum::TO_BASE:
        return ToBase::getInstance();
    case drone_state_enum::CHARGING:
        return Charging::getInstance();
    case drone_state_enum::DEAD:
        return Dead::getInstance();
    case drone_state_enum::DISCONNECTED:
        return Disconnected::getInstance();
    case drone_state_enum::RECONNECTED:
        return Reconnected::getInstance();
    default:
        throw std::invalid_argument("Invalid drone state");
    }
}

#endif //DRONESTATE_H
