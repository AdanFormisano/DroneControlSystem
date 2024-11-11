//
// Created by adan on 26/09/24.
//

#include "Buffer.h"
#include "../../utils/LogUtils.h"
#include <iostream>

void Buffer::WriteToBuffer(std::vector<DroneData> &data) {
    try {
        std::lock_guard lock(buffer_mutex);

        // std::cout << "DB writer Writing to buffer: " << data.size() << std::endl;

        // Bulk insert into buffer
        buffer.insert(buffer.end(), std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));
    } catch (const std::exception &e) {
        log_error("NewBuffer", "Error writing to the buffer: " + std::string(e.what()));
        // std::cerr << "Error writing to the buffer: " << e.what() << std::endl;
    }
}

std::vector<DroneData> Buffer::ReadFromBuffer() {
    try {
        std::lock_guard lock(buffer_mutex);

        // Bulk read from buffer
        std::vector data(std::make_move_iterator(buffer.begin()), std::make_move_iterator(buffer.end()));
        buffer.clear();
        return data;
    } catch (const std::exception &e) {
        // log_error("Error reading from the buffer: " + std::string(e.what()));
        log_error("NewBuffer", "Error reading from the buffer: " + std::string(e.what()));
        // std::cerr << "Error reading from the buffer: " << e.what() << std::endl;
        return {};
    }
}

size_t Buffer::getSize() {
    std::lock_guard lock(buffer_mutex);
    return buffer.size();
}