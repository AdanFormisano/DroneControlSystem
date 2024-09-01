/* checkTimeToReadDroneData(): This monitor checks that the amount of time that the system takes to read one set of
data entries from the drones is less than 2.24 seconds (this is the amount of time that a tick simulates).

 DroneControl reads the data from the Redis' stream every tick, but if it takes more time then the duration of the tick
 then the system will detect it; using IPC a message will be sent to the monitor to alert the user.

 The message queue will send int that rapresent which of the ticks took too much time.

 The monitor will save every tick that takes too long inside the failed_ticks vector.
 */
#include "Monitor.h"
#include <boost/interprocess/ipc/message_queue.hpp>

using namespace boost::interprocess;

void TimeToReadDataMonitor::checkTimeToReadData()
{
    try
    {
        spdlog::info("TIME-TO-READ-MONITOR: Initiated...");
        boost::this_thread::sleep_for(boost::chrono::seconds(10));

        // Create message queue
        message_queue mq(
            open_or_create  ,
            "time_to_read_data_monitor",
            100,
            sizeof(int)
            );

        // Params for messages
        message_queue::size_type recvd_size;
        unsigned int priority;

        while (true)
        {
            int failed_tick;    // Message container

            // Get number of messages in queue
            auto n_msg = mq.get_num_msg();
            spdlog::info("TIME-TO-READ-MONITOR: {} failed tick", n_msg);

            // Read every message sent by DC
            for ( auto i = 0; i < n_msg; i++)
            {
                // If the queue is empty tick_failed is false and
                if (auto tick_failed = mq.try_receive(&failed_tick, sizeof(failed_tick), recvd_size, priority); !tick_failed)
                {
                    spdlog::info("TIME-TO-READ-MONITOR: Everything is fine");
                } else  // Some tick took too long
                {
                    failed_ticks.emplace_back(failed_tick);
                    spdlog::warn("TIME-TO-READ-MONITOR: tick {} took too long!", failed_tick);
                    tick_last_read = failed_tick;
                }
            }

            if (!failed_ticks.empty())
            {
                pqxx::work W(db.getConnection());

                std::ostringstream oss;
                oss << ", ARRAY[";
                for (size_t i = 0; i < failed_ticks.size(); ++i) {
                    if (i != 0) oss << ",";
                    oss << failed_ticks[i];
                }
                oss << "]";
                auto q_array =  oss.str();

                // Insert into monitor_logs
                std::string q = "INSERT INTO monitor_logs (tick_n, time_to_read) "
                "VALUES (" + std::to_string(tick_last_read) + q_array + ") "
                "ON CONFLICT (tick_n) DO UPDATE SET "
                    "time_to_read = array_append(monitor_logs.time_to_read, " + q_array + ");";

                W.exec(q);
                W.commit();
            }


            boost::this_thread::sleep_for(boost::chrono::seconds(20));
        }
    } catch (interprocess_exception &ex)
    {
        spdlog::error("TIME-TO-READ-MONITOR: IPC error: {}", ex.what());
    }
}

