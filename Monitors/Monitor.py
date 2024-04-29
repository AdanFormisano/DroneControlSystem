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

print('End of simulation')
