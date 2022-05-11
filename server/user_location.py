import sqlite3
import datetime
user_db = '/var/jail/home/team39/databases/users_locations.db'

def request_handler(request):
    if request["method"] == "POST":
        lat = 0
        lon = 0
        station_name = ""
        try:
            username = request['form']['username']
            lat = float(request['form']['lat'])
            lon = float(request['form']['lon'])
        except Exception as e:
            return "Invalid inputs!"

        conn = sqlite3.connect(user_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS user_table (user text, lat real, lon reals, timing timestamp);''')

        if c.execute('''SELECT user from user_table WHERE user = ?;''', (username,)).fetchone() is None:
            c.execute('''INSERT into user_table VALUES (?,?,?,?);''', (username, lat, lon, datetime.datetime.now()))
        else:
            c.execute('''UPDATE user_table SET lat = ?, lon = ?, timing = ? WHERE user = ?;''', (lat, lon, datetime.datetime.now(),  username))
        conn.commit()
        conn.close()
        #things = c.execute('''SELECT * from user_table WHERE user = ?;''', (username,)).fetchone()
        #return things

    else:
        return "Only POSTS"
