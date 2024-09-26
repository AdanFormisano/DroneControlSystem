//
// Created by adan on 26/09/24.
//

#include "NewBuffer.h"

#include <iostream>

void NewBuffer::WriteToBuffer(std::vector<DroneData> &data) {
    try
    {
        std::lock_guard lock(buffer_mutex);

        // std::cout << "DB writer Writing to buffer: " << data.size() << std::endl;

        // Bulk insert into buffer
        buffer.insert(buffer.end(), std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error writing to the buffer: " << e.what() << std::endl;
    }
}

std::vector<DroneData> NewBuffer::ReadFromBuffer() {
    try
    {
        std::lock_guard lock(buffer_mutex);

        // Bulk read from buffer
        std::vector data(std::make_move_iterator(buffer.begin()), std::make_move_iterator(buffer.end()));
        buffer.clear();
        return data;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error reading from the buffer: " << e.what() << std::endl;
        return {};
    }
}

size_t NewBuffer::getSize() {
    std::lock_guard lock(buffer_mutex);
    return buffer.size();
}