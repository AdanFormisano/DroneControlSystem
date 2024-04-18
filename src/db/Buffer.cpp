#include "Buffer.h"

Buffer::~Buffer() {
    // Delete the queue
    std::queue<drone_data_ext>().swap(buffer);
}

void Buffer::WriteToBuffer(drone_data_ext &data) {
    try {
        boost::lock_guard<Buffer> write_lock(*this);
        buffer.push({data});
//        spdlog::info("Wrote to buffer Drone {} at tick {}", drone.id, tick_n);
    } catch (const std::exception &e) {
        spdlog::error("Error writing to the buffer: {}", e.what());
    }
}

drone_data_ext Buffer::ReadFromBuffer() {
    try {
        boost::lock_guard<Buffer> read_lock(*this);
        drone_data_ext data = buffer.front();
        buffer.pop();
        return data;
    } catch (const std::exception &e) {
        spdlog::error("Error reading from the buffer: {}", e.what());
        return {};
    }
}

size_t Buffer::getSize() {
    try {
        boost::lock_guard<Buffer> get_lock(*this);
        return !buffer.empty() ? buffer.size() : 0;
    } catch (const std::exception &e) {
        spdlog::error("Error getting the buffer size: {}", e.what());
        return 0;
    }
}

void Buffer::ClearBuffer() {
    try {
        boost::lock_guard<Buffer> clear_lock(*this);
        std::queue<drone_data_ext>().swap(buffer);
    } catch (const std::exception &e) {
        spdlog::error("Error clearing the buffer: {}", e.what());
    }

}

void MiniBuffer::WriteBlockToDB(Database &db, int size) {
    try {
        // Base query string
        std::string sql = "INSERT INTO drone_logs (tick_n, drone_id, status, charge, x, y, checked) VALUES ";

        // Creates the entire query string
        for (size_t _ = 0; _ < size; ++_) {
            drone_data_ext data = ReadFromBuffer();

            // This print verifies that the minibuffer is full and that the second thread is reading from it
//            spdlog::info("Writing to DB Drone {} at tick {}", data.data.id, data.tick_n);

            sql += "(" + std::to_string(data.tick_n) + ", " +
                   std::to_string(data.data.id) + ", '" +
                   data.data.status + "', '" +
                   std::to_string(data.data.charge) + "', " +
                   std::to_string(data.data.position.first) + ", " +
                   std::to_string(data.data.position.second) + ", '" +
                   (data.check ? "true" : "false") + "')";
            if (_ != size - 1) {
                sql += ", ";
            }
        }
        // Add semicolon to the end of the query
        sql += ";";

//        spdlog::info("Query: {}", sql);

        db.ExecuteQuery(sql);
    } catch (const std::exception &e) {
        spdlog::error("Error writing to the database: {}", e.what());
    }

//    std::ostringstream oss;
//    for(const auto& val : ids) {
//        oss << val << " ";
//    }
//
//    spdlog::info("Drones IDs added to DB: {}", oss.str());
//    ids.clear();
}

//std::shared_ptr<MiniBuffer> MiniBufferContainer::GetMiniBuffer(int key) {
//    return mini_buffers[key];
//}
//
//std::shared_ptr<MiniBuffer> MiniBufferContainer::GetFirstMiniBuffer() {
//    return mini_buffers.begin()->second;
//}
//
//void MiniBufferContainer::CreateMiniBuffer(int key) {
//    mini_buffers[key] = std::make_shared<MiniBuffer>(key);
//}
//
//void MiniBufferContainer::DeleteFirstMiniBuffer() {
//    mini_buffers.erase(mini_buffers.begin());
//}

// Function ran by the thread to dispatch the drone data to the mini buffers
void DispatchDroneData(Buffer &buffer, MiniBufferContainer &mini_buffers) {
    // TODO: Add better sync mechanism: DroneControl needs to have the priority on the buffer
    while (true) {
        auto buffer_size = buffer.getSize();
        if (buffer_size > 0) {
            std::vector<drone_data_ext> data_vector;
            // Create boost guard
            for (size_t _ = 0; _ < buffer_size; ++_) {
//                drone_data_ext data = buffer.ReadFromBuffer();
//                spdlog::info("Reading tick_n {} from BIG BUFFER", data.tick_n);
                data_vector.push_back(buffer.ReadFromBuffer());
            }
            spdlog::info("{} elements read from BIG BUFFER", data_vector.size());

            boost::lock_guard<MiniBufferContainer> mini_lock(mini_buffers);
            for (auto &data : data_vector) {
//                spdlog::info("Reading Drone {} at tick {} from buffer", data.data.id, data.tick_n);

                // Check if minibuffer is not nullptr
                // Check if the minibuffer already exists
                if (mini_buffers.mini_buffers.contains(data.tick_n)) {
                    // mini_buffers[data.tick_n].WriteToBuffer(data.data, data.check, data.tick_n);
                    mini_buffers.mini_buffers[data.tick_n]->WriteToBuffer(data);
                } else {
                    // MiniBuffer mini_bffr(data.tick_n);
                    // mini_buffers[index] = mini_bffr;
                    mini_buffers.mini_buffers[data.tick_n] = std::make_shared<MiniBuffer>(data.tick_n);
                    mini_buffers.mini_buffers[data.tick_n]->WriteToBuffer(data);
                }
            }
            mini_buffers.cv.notify_all();
        }
        boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
    }
};

// Function ran by the thread to write the mini buffer to the database
void WriteToDB(MiniBufferContainer &mini_buffers, Database &db) {
    // TODO: Execute while the program is running but before the program ends, write the remaining mini buffers
    while (true) {
//        spdlog::info("Numbers of mini buffers: {}", mini_buffers.size());
        // Create boost guard
        boost::lock_guard<MiniBufferContainer> mini_lock(mini_buffers);
        mini_buffers.cv.wait(static_cast<boost::unique_lock <boost::mutex> &>(mini_lock), [&mini_buffers] { return !mini_buffers.mini_buffers.empty(); });
            // Check if the first mini buffer is full
//            int tick = mini_buffers.mini_buffers.begin()->first;
//            int max_size = static_cast<int>(std::floor(tick / 5)+1) * 150;
//
//            return !mini_buffers.mini_buffers.empty();
        auto mini_b = mini_buffers.mini_buffers.begin()->second;

        if (mini_b == nullptr) {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
            continue;
        } else {
            // Check if the first mini buffer is full
            int tick = mini_buffers.mini_buffers.begin()->first;
            int max_size = static_cast<int>(std::floor(tick / 5)+1) * 150;
            spdlog::info("Mini buffer {} is size: {}", mini_buffers.mini_buffers.begin()->first, mini_b->getSize());
            if (mini_b->getSize() == max_size && tick < 20) {
                spdlog::info("Writing minibuffer {} of size {} to db", mini_buffers.mini_buffers.begin()->first, mini_b->getSize());
                mini_b->WriteBlockToDB(db, max_size);

                // Minibuffer is now empty and can be deleted
                mini_buffers.mini_buffers.erase(mini_buffers.mini_buffers.begin());
            } else if (mini_b->getSize() == ZONE_NUMBER && tick >= 20){
                mini_b->WriteBlockToDB(db, ZONE_NUMBER);
                spdlog::info("Writing minibuffer {} of size {} to db", mini_buffers.mini_buffers.begin()->first, mini_b->getSize());

                // Minibuffer is now empty and can be deleted
                mini_buffers.mini_buffers.erase(mini_buffers.mini_buffers.begin());
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
        }
    }
};