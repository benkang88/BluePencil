from datetime import datetime
import sqlite3
import random
pencil_data = "/var/jail/home/team39/borrowed_pencils.db"
code_data = "/var/jail/home/team39/codes.db"
station_data = "/var/jail/home/team39/stations.db"

def request_handler(request):
    create_code_database()
    create_pencil_database()
    create_station_database()
    if request["method"] == "POST":
        try:
            user = request['form']['username']
            station = request['form']['station']
            code = create_code(user, station)
            if code is None:
                return "Station is empty"
            return code
        except Exception as e:
            return "Error: username is missing"
    else:
        try:
            station = request['values']['station']
            code = request['values']['code']
            user = search_code(station, code)
            if user is not None:
                insert_into_pencil_database(user, station)
                return "Unlocked!"
            return "incorrect code"
        except Exception as e:
            return "Error: username is missing"

def create_pencil_database():
    conn = sqlite3.connect(pencil_data)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS pencil_data (user text, start timestamp);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database

def create_code_database():
    conn = sqlite3.connect(code_data)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS code_data (user text, code text, station text, start timestamp);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database

def create_station_database():
    conn = sqlite3.connect(station_data)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS code_data (station text, num_pencils int);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database

def insert_into_pencil_database(user, station):
    conn = sqlite3.connect(pencil_data)
    c = conn.cursor()
    c.execute('''INSERT into pencil_data VALUES (?,?);''', (user, datetime.now())) # pencil is now being borrowed
    conn.commit()
    conn.close()
    conn = sqlite3.connect(station_data)
    c = conn.cursor()
    c.execute('''UPDATE station_data SET num_pencils = num_pencils - 1 WHERE station = ?;''', (station,)) # decrement pencils at station
    conn.commit()
    conn.close()

def create_code(user, station):
    conn = sqlite3.connect(station_data)
    c = conn.cursor()
    available_pencils = int(c.execute('''SELECT num_pencils FROM station_data WHERE station = ?;''', (station,)).fetchone()[0]) # check that pencil is available
    if available_pencils == 0:
        return None
    conn.commit()
    conn.close()
    conn = sqlite3.connect(code_data)
    c = conn.cursor()
    while True:
        code = ""
        for i in range(5):
            code += str(random.randint(1,3))
        if c.execute('''SELECT user FROM code_data WHERE code = ? AND station = ?;''', (code, station)).fetchone() is None: # no duplicate code at same station
            c.execute('''INSERT into code_data VALUES (?,?,?,?);''', (user, code, station, datetime.now()))
            break
    conn.commit()
    conn.close()
    return code

def search_code(station, code):
    conn = sqlite3.connect(code_data)
    c = conn.cursor()
    thirty_seconds_ago = datetime.datetime.now() - datetime.timedelta(seconds = 30) # create time for 30 seconds ago!
    c.execute('''DELETE FROM user_data WHERE start < ?;''', (thirty_seconds_ago,)) # remove old codes
    user = c.execute('''SELECT user FROM code_data WHERE station = ? AND code = ?;''', (station, code)).fetchone()
    conn.commit()
    conn.close()
    if user is not None:
        return user[0]
    return None
