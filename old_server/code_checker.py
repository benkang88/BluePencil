import sqlite3
import datetime
checkout_db = '/var/jail/home/team39/checkout.db'

def request_handler(request):
    if request["method"] == "GET":
        station_name = ""
        first = 0
        second = 0
        third = 0
        try:
            station_name = request["values"]["station"]
            first = int(request["values"]["first"])
            second = int(request["values"]["second"])
            third = int(request["values"]["third"])
        except Exception as e:
            return "Invalid inputs!"

        conn = sqlite3.connect(checkout_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS checkout_codes (user text, station text, first int, second int, third int, timing timestamp);''')

        one_minute_ago = datetime.datetime.now() - datetime.timedelta(seconds = 60)

        request = c.execute('''SELECT * FROM checkout_codes WHERE timing > ? AND first = ? AND second = ? AND third = ? ORDER BY timing DESC;''', (one_minute_ago, first, second, third)).fetchone()
        conn.commit()
        conn.close()

        if request != None:
            return "YES"
        else:
            return "NO"