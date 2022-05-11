from datetime import datetime
import sqlite3
station_data = "/var/jail/home/team39/stations.db"

def request_handler(request):
    if request["method"] == "POST":
        conn = sqlite3.connect(station_data)  # connect to that database (will create if it doesn't already exist)
        c = conn.cursor()  # move cursor into database (allows us to execute commands)
        c.execute('''CREATE TABLE IF NOT EXISTS station_data (station text, lat real, lon real);''') # run a CREATE TABLE command
        try:
            station = request['form']['station']
            lat = float(request['form']['lat'])
            lon = float(request['form']['lon'])
            c.execute('''INSERT into station_data VALUES (?,?, ?);''', (station, lat, lon)) #
            conn.commit() # commit commands (VERY IMPORTANT!!)
            conn.close() # close connection to database
            return insert_into_database(station, num_pencils)
        except Exception as e:
            return "Error: station or pencils is missing"
    else:
        return "No GETs"

