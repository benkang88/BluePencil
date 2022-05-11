import sqlite3
import datetime
checkout_db = '/var/jail/home/team39/databases/checkout.db'
user_data = "/var/jail/home/team39/databases/user_data.db" # just come up with name of database
pencil_db = '/var/jail/home/team39/databases/borrowed_pencils.db'


def request_handler(request):
    if request["method"] == "GET":
        station_name = ""
        first = 0
        second = 0
        third = 0
        user = ""
        try:
            station_name = request["values"]["station"]
            first = int(request["values"]["first"])
            second = int(request["values"]["second"])
            third = int(request["values"]["third"])
        except Exception as e:
            return "Invalid inputs!"

        conn = sqlite3.connect(checkout_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS checkout_codes (user text, station text, first int, second int, third int, timing timestamp, used int);''')

        one_minute_ago = datetime.datetime.now() - datetime.timedelta(seconds = 60)

        request = c.execute('''SELECT * FROM checkout_codes WHERE first = ? AND second = ? AND third = ? ORDER BY timing DESC;''', (first, second, third)).fetchone()

        if request != None:
            time = request[5]
            code_timestamp = datetime.datetime.strptime(time, '%Y-%m-%d %H:%M:%S.%f')
            if request[6] == 0 and code_timestamp >  one_minute_ago:
                c.execute('''UPDATE checkout_codes SET used = 1 WHERE first = ? AND second = ? AND third = ? AND station = ?;''', (first, second, third, station_name)) 
                conn.commit()
                conn.close()    
                user = request[0]
                conn = sqlite3.connect(pencil_db)
                c = conn.cursor()
                c.execute('''CREATE TABLE IF NOT EXISTS pencils (user text, timing timestamp);''')

                c.execute('''INSERT into pencils VALUES (?,?);''', (user, datetime.datetime.now()))
                conn.commit()
                conn.close()
                return "YES"
        conn.commit()
        conn.close()    
        return "NO"

    
