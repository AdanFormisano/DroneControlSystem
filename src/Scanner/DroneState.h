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

class Idle final : public DroneState
{
public:
    void enter(Drone* drone) override {};
    void run(Drone* drone) override;
    void exit(Drone* drone) override {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::IDLE; }

private:
    Idle() = default;
    Idle(const Idle& other) = default;
    Idle& operator=(const Idle& other) = default;
};

class ToStartingLine final : public DroneState
{
public:
    void enter(Drone* drone) override;
    void run(Drone* drone) override;
    void exit(Drone* drone) override;
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::TO_STARTING_LINE; }

private:
    ToStartingLine() = default;
    ToStartingLine(const ToStartingLine& other) = default;
    ToStartingLine& operator=(const ToStartingLine& other) = default;
};

class Ready final : public DroneState
{
public:
    void enter(Drone* drone) override;
    void run(Drone* drone) override;
    void exit(Drone* drone) override {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::READY; }

private:
    Ready() = default;
    Ready(const Ready& other) = default;
    Ready& operator=(const Ready& other) = default;
};

class Working final : public DroneState
{
public:
    void enter(Drone* drone) override;
    void run(Drone* drone) override;
    void exit(Drone* drone) override {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::WORKING; }

private:
    Working() = default;
    Working(const Working& other) = default;
    Working& operator=(const Working& other) = default;
};

class ToBase final : public DroneState
{
public:
    void enter(Drone* drone) override;
    void run(Drone* drone) override;
    void exit(Drone* drone) override;
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::TO_BASE; }

private:
    ToBase() = default;
    ToBase(const ToBase& other) = default;
    ToBase& operator=(const ToBase& other) = default;
};

class Charging final : public DroneState
{
public:
    void enter(Drone* drone) override;
    void run(Drone* drone) override{};
    void exit(Drone* drone) override{};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::CHARGING; }

private:
    Charging() = default;
    Charging(const Charging& other) = default;
    Charging& operator=(const Charging& other) = default;
};

class Dead final : public DroneState
{
public:
    void enter(Drone* drone) override;
    void run(Drone* drone) override;
    void exit(Drone* drone) override {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::DEAD; }

private:
    Dead() = default;
    Dead(const Dead& other) = default;
    Dead& operator=(const Dead& other) = default;
};

class Disconnected final : public DroneState
{
public:
    void enter(Drone* drone) override;

    static void hidden_to_starting_line(Drone* drone);
    static void hidden_ready(Drone* drone);
    static void hidden_working(Drone* drone);
    static void hidden_to_base(Drone* drone);

    void exit(Drone* drone) override {};
    void run(Drone* drone) override;
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::DISCONNECTED; }

private:
    Disconnected() = default;
    Disconnected(const Disconnected& other) = default;
    Disconnected& operator=(const Disconnected& other) = default;
};

class Reconnected final : public DroneState
{
public:
    void enter(Drone* drone) override;
    void exit(Drone* drone) override {};
    void run(Drone* drone) override;
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::RECONNECTED; }

private:
    Reconnected() = default;
    Reconnected(const Reconnected& other) = default;
    Reconnected& operator=(const Reconnected& other) = default;
};

class HighConsumption : public DroneState
{
public:
    void enter(Drone* drone) override {};
    void exit(Drone* drone) override {};
    static DroneState& getInstance();
    drone_state_enum getState() override { return drone_state_enum::NONE; }

private:
    HighConsumption() = default;
    HighConsumption(const HighConsumption& other);
    HighConsumption& operator=(const HighConsumption& other);
};

inline DroneState& getDroneState(const drone_state_enum state)
{
    switch (state)
    {
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

#endif // DRONESTATE_H
