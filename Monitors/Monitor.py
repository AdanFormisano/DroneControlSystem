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
import time

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

last_element_read = None
i = 0

drones_charge_times = {}    # Dictionary to store the start and end tick of the drones' charging time


def get_latest_element():
    cursor.execute("SELECT * FROM drone_logs ORDER BY tick_n DESC LIMIT 1")

    latest_element = cursor.fetchone()

    if latest_element is not None:
        return latest_element
    else:
        return None

def check_new_data(last_element):
    latest_element = get_latest_element()
    if latest_element is not last_element:
        print(f'tick_n: {latest_element[0]}, drone_id: {latest_element[1]}, zone: {latest_element[4]}')
        last_element = latest_element
    else:
        print('No new data')

    time.sleep(5)

while i < 10:
    i += 1
    check_new_data(last_element_read)

# checkDroneReachargeTime() monitor implementation DOABLE
def checkDroneRechargeTime():
    """
    This monitor checks that the number of ticks that a drone has been charging is between [2,3] hours.
    """
    # Checks if there are new drones that are charging
    cursor.execute("SELECT * FROM drone_logs WHERE status = 'CHARGING'")
    for row in cursor:
        if row[1] not in drones_charge_times:
            drones_charge_times[row[1]] = [row[0]]  # Save the start tick of the charging time
        else:


    time.sleep(300)

# checkTimeToReadDroneData() monitor implementation DOABLE BUT NEEDS CHANGES IN THE C++ CODE (new data in DB from DC)

#checkDroneCharge() monitor implementation DOABLE

print('End of simulation')
