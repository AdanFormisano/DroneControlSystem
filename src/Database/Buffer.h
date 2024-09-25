#ifndef DRONECONTROLSYSTEM_BUFFER_H
#define DRONECONTROLSYSTEM_BUFFER_H
#include "../globals.h"
#include "Database.h"
#include <boost/thread.hpp>
#include <boost/thread/lockable_adapter.hpp>
#include <condition_variable>
#include <pqxx/pqxx>
#include "sw/redis++/redis++.h"

using namespace sw::redis;

class Buffer : public boost::basic_lockable_adapter<boost::mutex> {
public:
    ~Buffer();

    void WriteToBuffer(DroneData& data);
    void WriteBatchToBuffer(std::vector<DroneData>& data);
    DroneData ReadFromBuffer();
    std::vector<DroneData> GetAllData();
    void ClearBuffer();
    void WriteFault(drone_fault &fault) { faults.push_back(fault); }
    std::vector<drone_fault> GetFaults() { return faults; }
    void ClearFaults() { faults.clear(); }

    size_t getSize();
    size_t getFaultsSize() { return faults.size(); }

private:
    std::queue<DroneData> buffer;
    std::vector<drone_fault> faults;
};
#endif // DRONECONTROLSYSTEM_BUFFER_H
