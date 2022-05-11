from datetime import datetime
import sqlite3
station_data = "/var/jail/home/team39/stations.db"

def request_handler(request):
    create_station_database()
    if request["method"] == "POST":
        try:
            station = request['form']['station']
            num_pencils = int(request['form']['num_pencils'])
            lat = float(request['form']['lat'])
            lon = float(request['form']['lon'])
            return insert_into_database(station, lat, lon, num_pencils)
            
        except Exception:
            return "Error: station or pencils is missing"
    else:
        return "No GETs"

def create_station_database():
    conn = sqlite3.connect(station_data)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS station_data (station text, lat real, lon real, num_pencils int);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database


def insert_into_database(station, lat, lon, num_pencils):
    conn = sqlite3.connect(station_data)
    c = conn.cursor()
    c.execute('''INSERT into station_data VALUES (?,?,?,?);''', (station, lat, lon, num_pencils)) # 
    conn.commit()
    conn.close()
    return f"Station {station} at {lat} latitude, {lon} longitude, with {num_pencils} pencils successfully added"
