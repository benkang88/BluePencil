import sqlite3
import datetime
import random
checkout_db = '/var/jail/home/team39/databases/checkout.db'
user_data = "/var/jail/home/team39/databases/user_data.db"

def request_handler(request):
    if request["method"] == "GET":
        user = ""
        station_name = ""
        try:
            user = request["values"]["user"]
            station_name = request["values"]["station"]
        except Exception as e:
            return "Invalid inputs!"

        first = random.randint(1, 3)
        second = random.randint(1, 3)
        third = random.randint(1, 3)

        conn = sqlite3.connect(checkout_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS checkout_codes (user text, station text, first int, second int, third int, timing timestamp, used int);''')

        c.execute('''INSERT into checkout_codes VALUES (?,?,?,?,?,?,?);''', (user, station_name, first, second, third, datetime.datetime.now(),0))

        conn.commit()
        conn.close()
        stats(user, station_name)
        return str(first) + str(second) + str(third)

def stats(user, station):
    conn = sqlite3.connect(user_data)
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS user_stats (user text, timing timestamp, checkouts int, last_station text);''')
    c.execute('''UPDATE user_stats SET last_station = ? WHERE user = ?;''', (station, user))
    conn.commit()
    conn.close()
