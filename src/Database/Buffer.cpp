#include <set>
#include <iostream>
#include "Buffer.h"

Buffer::~Buffer()
{
    // Delete the queue
    std::queue<DroneData>().swap(buffer);
}

void Buffer::WriteToBuffer(DroneData& data)
{
    try
    {
        boost::lock_guard<Buffer> write_lock(*this);
        buffer.push({data});
        //        spdlog::info("Wrote to buffer Drone {} at tick {}", drone.id, tick_n);
    }
    catch (const std::exception& e)
    {
        // spdlog::error("Error writing to the buffer: {}", e.what());
        std::cerr << "Error writing to the buffer: " << e.what() << std::endl;
    }
}

void Buffer::WriteBatchToBuffer(std::vector<DroneData>& data)
{
    try
    {
        boost::lock_guard<Buffer> write_lock(*this);
        for (auto& d : data)
        {
            buffer.push({d});
        }
    }
    catch (const std::exception& e)
    {
        // spdlog::error("Error writing to the buffer: {}", e.what());
        std::cerr << "Error writing to the buffer: " << e.what() << std::endl;
    }
}

DroneData Buffer::ReadFromBuffer()
{
    try
    {
        boost::lock_guard<Buffer> read_lock(*this);
        DroneData data = buffer.front();
        buffer.pop();
        return data;
    }
    catch (const std::exception& e)
    {
        // spdlog::error("Error reading from the buffer: {}", e.what());
        std::cerr << "Error reading from the buffer: " << e.what() << std::endl;
        return {};
    }
}

std::vector<DroneData> Buffer::GetAllData()
{
    try
    {
        boost::lock_guard<Buffer> read_lock(*this);
        std::vector<DroneData> data_vector;
        while (!buffer.empty())
        {
            data_vector.push_back(buffer.front());
            buffer.pop();
        }
        return data_vector;
    }
    catch (const std::exception& e)
    {
        // spdlog::error("Error reading from the buffer: {}", e.what());
        std::cerr << "Error reading from the buffer: " << e.what() << std::endl;
        return {};
    }
}

size_t Buffer::getSize()
{
    try
    {
        boost::lock_guard<Buffer> get_lock(*this);
        return !buffer.empty() ? buffer.size() : 0;
    }
    catch (const std::exception& e)
    {
        // spdlog::error("Error getting the buffer size: {}", e.what());
        std::cerr << "Error getting the buffer size: " << e.what() << std::endl;
        return 0;
    }
}

void Buffer::ClearBuffer()
{
    try
    {
        boost::lock_guard<Buffer> clear_lock(*this);
        std::queue<DroneData>().swap(buffer);
    }
    catch (const std::exception& e)
    {
        // spdlog::error("Error clearing the buffer: {}", e.what());
        std::cerr << "Error clearing the buffer: " << e.what() << std::endl;
    }
}
