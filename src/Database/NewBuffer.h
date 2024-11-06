#ifndef NEWBUFFER_H
#define NEWBUFFER_H
#include <deque>
#include <mutex>

#include "../globals.h"


class NewBuffer {
public:
    void WriteToBuffer(std::vector<DroneData> &data);
    std::vector<DroneData> ReadFromBuffer();

    size_t getSize();

private:
    std::mutex buffer_mutex;
    std::deque<DroneData> buffer;
};



#endif //NEWBUFFER_H
