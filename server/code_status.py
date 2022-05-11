import sqlite3
import datetime
checkout_db = '/var/jail/home/team39/databases/checkout.db'

def request_handler(request):
    if request["method"] == "GET":
        user = ""
        first = 0
        second = 0
        third = 0
        try:
            user = request["values"]["user"]
            first = int(request["values"]["first"])
            second = int(request["values"]["second"])
            third = int(request["values"]["third"])
        except Exception as e:
            return "Invalid inputs!"
        conn = sqlite3.connect(checkout_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS checkout_codes (user text, station text, first int, second int, third int, timing timestamp, used int);''')

        value = c.execute('''SELECT used, timing FROM checkout_codes WHERE user = ? AND first = ? AND second = ? AND third = ? ORDER BY timing DESC;''', (user, first, second, third)).fetchone()

        conn.commit()
        conn.close()
        used = value[0]
        try:
            code_timestamp_string = value[1]
            code_timestamp = datetime.datetime.strptime(code_timestamp_string, '%Y-%m-%d %H:%M:%S.%f')
        except Exception as e:
            return "Error parsing code_timestamp: " + code_timestamp_string
        one_minute_ago = datetime.datetime.now() - datetime.timedelta(seconds = 60)
        if one_minute_ago > code_timestamp:
            return "EXPIRED"
        elif used == 1:
            return "USED"
        else:
            return "NOT USED"
