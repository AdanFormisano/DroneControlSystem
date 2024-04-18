#ifndef DRONECONTROLSYSTEM_BUFFER_H
#define DRONECONTROLSYSTEM_BUFFER_H
#include "../globals.h"
#include "Database.h"
#include <boost/thread.hpp>
#include <condition_variable>
#include <pqxx/pqxx>
#include <boost/thread/lockable_adapter.hpp>

struct drone_data_ext {
    drone_data data;
    bool check;
    int tick_n;
};

class MiniBuffer;

class Buffer : public boost::basic_lockable_adapter<boost::mutex> {
public:
    ~Buffer();

    void WriteToBuffer(drone_data_ext &data);
    drone_data_ext ReadFromBuffer();
    void ClearBuffer();

    size_t getSize();

private:
    std::queue<drone_data_ext> buffer;
};

class MiniBuffer : public Buffer {
public:
    explicit MiniBuffer(int tick_n) : id(tick_n) {}

    void WriteBlockToDB(Database &db, int size);

    int getID() const { return id; }

private:
    int id;
};

class MiniBufferContainer : public boost::basic_lockable_adapter<boost::mutex> {
public:
    std::map<int, std::shared_ptr<MiniBuffer>> mini_buffers;
    boost::condition_variable cv;
};

void DispatchDroneData(Buffer &buffer, MiniBufferContainer &mini_buffers);
void WriteToDB(MiniBufferContainer &mini_buffers, Database &db);
#endif //DRONECONTROLSYSTEM_BUFFER_H
