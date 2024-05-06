"""
This monitor checks that the number of ticks that a drone has been charging is between [2,3] hours.
"""

import psycopg2
import time

drones_charge_times = {}    # Dictionary to store the start and end tick of the drones' charging time

def check_drone_recharge_time(cursor: psycopg2.extensions.cursor):
    while True:
        print(f"Checking drones' charging time...")
        # Get the drones that are charging
        get_charging_drones(cursor)

        # Get the drones that are charged
        get_charged_drones(cursor)

        # Check the drones' charging time
        for drone_id, charge_times in drones_charge_times.items():
            if len(charge_times) != 2:
                print(f'Drone {drone_id} is still charging...')
                continue
            else:
                start_tick, end_tick = charge_times
                delta_time = end_tick - start_tick

                if 3214 <= delta_time <= 4821:
                    print(f'Drone {drone_id} has been charging for {delta_time} ticks: {(delta_time * 2.24) / 60} minutes')
                else:
                    print(f'[WARNING] Drone {drone_id} has been charging for {delta_time} ticks: {(delta_time * 2.24) / 60} minutes')

        print("Going to sleep...")
        time.sleep(15)

def get_charging_drones(cursor: psycopg2.extensions.cursor):
    cursor.execute("SELECT drone_id, tick_n FROM drone_logs WHERE status = 'CHARGING'")
    drones_in_charge = cursor.fetchall()

    for drone_id, start_tick in drones_in_charge:
        if drone_id not in drones_charge_times:
            drones_charge_times[drone_id] = [start_tick]  # Save the start tick

def get_charged_drones(cursor: psycopg2.extensions.cursor):
    cursor.execute("SELECT drone_id, tick_n FROM drone_logs WHERE status = 'CHARGE_COMPLETE'")
    drones_charged = cursor.fetchall()

    for drone_id, end_tick in drones_charged:
        if drone_id in drones_charge_times:
            drones_charge_times[drone_id].append(end_tick)
        else:
            # TODO: Add a check for 'WAITING_FOR_CHARGE' status
            print(f'Drone {drone_id} is not charging')
