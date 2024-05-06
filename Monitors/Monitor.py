"""
All monitors are going to be executed in parallel during the simulation. In particular there are 3 functional monitors
and 2 non-functional monitors.

The functional monitors are:
    - checkDroneRechargeTime(): This monitor checks that the number of ticks that a drone has been charging is between
        [2,3] hours.
    - checkAreaCoverage(): This monitor checks that all the points of the 6x6 Km area are verified (covered by the drones)
        every 5 minutes.
    - checkZoneVerification(): This monitor checks that each zone is verified every 5 minutes (this has some internal
        implications).

The non-functional monitors are:
    - checkTimeToReadDroneData(): This monitor checks that the amount of time that the system takes to read one set of
        data entries from the drones is less than 2.24 seconds (this is the amount of time that a tick simulates).
    - checkDroneCharge(): This monitor checks that the drones' charge is correctly managed by the system. This means that
        the drones are swapped when their charge is just enough to return to the base.
"""
import psycopg2

import RechargeTime

conn = psycopg2.connect(database="dcs",
                        host="localhost",
                        user="postgres",
                        password="admin@123",
                        port="5432")

cursor = conn.cursor()

# cursor.execute("SELECT * FROM drone_logs WHERE tick_n = 1")
#
# for row in cursor:
#     print(f'drone_id: {row[1]}, zone: {row[4]}')

# This monitor is going to be executed during the simulation

# Check if there is new data in the DB that needs to be processed
# checkTimeToReadDroneData() monitor implementation DOABLE BUT NEEDS CHANGES IN THE C++ CODE (new data in DB from DC)
#checkDroneCharge() monitor implementation DOABLE

def main():
    # from threading import Thread

    # Thread(target=check_new_data, args=(cursor,)).start()
    # Thread(target=checkDroneRechargeTime, args=(cursor,)).start()
    # Thread(target=checkAreaCoverage, args=(cursor,)).start()
    # Thread(target=checkZoneVerification, args=(cursor,)).start()
    # Thread(target=checkTimeToReadDroneData, args=(cursor,)).start()
    # Thread(target=checkDroneCharge, args=(cursor,)).start()

    print('Launching monitors...')

    RechargeTime.check_drone_recharge_time(cursor)


    conn.close()

if __name__ == '__main__':
    main()
