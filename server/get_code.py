import sqlite3
import datetime
import random
checkout_db = '/var/jail/home/team39/checkout.db'

def request_handler(request):
    if request["method"] == "GET":
        user = ""
        station_name = ""
        try:
            user = request["values"]["user"]
            station_name = request["values"]["station"]
        except Exception as e:
            return "Invalid inputs!"

        first = random.randint(0, 9)
        second = random.randint(0, 9)
        third = random.randint(0, 9)

        conn = sqlite3.connect(checkout_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS checkout_codes (user text, station text, first int, second int, third int, timing timestamp);''')

        c.execute('''INSERT into checkout_codes VALUES (?,?,?,?,?,?);''', (user, station_name, first, second, third, datetime.datetime.now()))

        conn.commit()
        conn.close()
        return str(first) + str(second) + str(third)