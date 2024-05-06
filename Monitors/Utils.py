import psycopg2
import time

global last_element_read
def check_new_data(cursor: psycopg2.extensions.cursor):
    global last_element_read

    cursor.execute("SELECT * FROM drone_logs ORDER BY tick_n DESC LIMIT 1")
    element_read = cursor.fetchone()

    if element_read is not last_element_read:
        print(f'tick_n: {element_read[0]}, drone_id: {element_read[1]}, zone: {element_read[4]}')
        last_element_read = element_read
    else:
        print('No new data')

    time.sleep(5)