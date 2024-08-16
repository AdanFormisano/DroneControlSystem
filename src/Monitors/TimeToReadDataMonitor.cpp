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

void TimeToReadDataMonitor::checktimeToReadData()
{
    try
    {
        spdlog::info("TIME_TO_READ_DATA_MONITOR initiated...");

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
            spdlog::info("TIME_TO_READ_DATA_MONITOR: {} of failed tick", n_msg);

            // Read every message sent by DC
            for ( auto i = 0; i < n_msg; i++)
            {
                auto tick_failed = mq.try_receive(&failed_tick, sizeof(failed_tick), recvd_size, priority);

                // If the queue is empty tick_failed is false and
                if (!tick_failed)
                {
                    spdlog::info("TIME_TO_READ_DATA_MONITOR: Everything is fine", failed_tick);
                } else  // Some tick took too long
                {
                    failed_ticks.emplace_back(failed_tick);
                    spdlog::warn("TIME_TO_READ_DATA_MONITOR: tick {} took too long!", failed_tick);
                }
            }


            boost::this_thread::sleep_for(boost::chrono::seconds(20));
        }
    } catch (interprocess_exception &ex)
    {
        spdlog::error("IPC error: {}", ex.what());
    }
}

