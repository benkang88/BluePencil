import sqlite3
import datetime
import random
checkout_db = '/var/jail/home/team39/databases/checkout.db'

now = datetime.datetime.now()

centers = {
    "Student Center": (-71.095, 42.359),
    "Dorm Row": (-71.098, 42.356),
    "Simmons/Briggs": (-71.102, 42.356),
    "Boston FSILG (West)": (-71.112, 42.349),
    "Boston FSILG (East)": (-71.087, 42.35),
    "Stata/North Court": (-71.09, 42.362),
    "East Campus": (-71.089, 42.36),
    "Vassar Academic Buildings": (-71.093, 42.361),
    "Infinite Corridor/Killian": (-71.092, 42.359),
    "Kendall Square": (-71.086, 42.362),
    "Sloan/Media Lab": (-71.085, 42.361),
    "North Campus": (-71.101, 42.36),
    "Technology Square": (-71.091, 42.364),

}

def distance(coord1, coord2):
    return ((coord1[0]-coord2[0])**2 + (coord1[1]-coord2[1])**2)**.5

def request_handler(request):
    if request["method"] == "POST":
        lat = 0
        lon = 0
        try:
            user = request['form']['user']
            lat = float(request['form']['lat'])
            lon = float(request['form']['lon'])
        except Exception as e:
            return "Invalid inputs!"

        conn = sqlite3.connect(checkout_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS checkout_request_table (user text, lat real, lon real, first int, second int, third int, timing timestamp);''')
        while True:
            first = random.randint(0, 9)
            second = random.randint(0, 9)
            third = random.randint(0, 9)
            if c.execute('''SELECT user FROM checkout_request_table WHERE first = ? AND second = ? AND third = ?;''', (first, second, third)).fetchone() is None:
                c.execute('''INSERT into checkout_request_table VALUES (?,?,?,?,?,?,?);''', (user, lat, lon, first, second, third, datetime.datetime.now()))
                break
        # thirty_seconds_ago = datetime.datetime.now() - datetime.timedelta(seconds = 30)
        # things = c.execute('''SELECT * FROM checkout_request_table WHERE timing > ? ORDER BY timing DESC;''', (thirty_seconds_ago,)).fetchone()
# WHERE timing > ? ORDER BY timing ASC;''',(thirty_seconds_ago,)).fetchall()
        outs = ""
        for x in [first, second, third]:
            outs+=str(x) + "\n"
        conn.commit()
        conn.close()
        return outs
        
    else:
        first = 0
        second = 0
        third = 0
        user = ""
        lat = 0
        lon = 0
        try:
            # user = request['values']['station']
            lat = float(request['values']['lat'])
            lon = float(request['values']['lon'])
            first = int(request['values']['first'])
            second = int(request['values']['second'])
            third = int(request['values']['third'])
        except Exception as e:
            return "Invalid inputs!"
        conn = sqlite3.connect(checkout_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS checkout_request_table (user text, lat real, lon real, first int, second int, third int, timing timestamp);''')
        thirty_seconds_ago = datetime.datetime.now() - datetime.timedelta(seconds = 30)
        things = c.execute('''SELECT * FROM checkout_request_table WHERE timing > ? AND first = ? AND second = ? AND third = ? ORDER BY timing DESC;''', (thirty_seconds_ago, first, second, third)).fetchone()
        conn.commit()
        conn.close()
        if things == None:
            return "Code submission timeout"
        user = things[0]
        real_lat = things[1]
        real_lon = things[2]
        real_first = int(things[3])
        real_second = int(things[4])
        real_third = int(things[5])
        
        dist = distance((lat, lon), (real_lat, real_lon))
        if dist < 0.5 and first == real_first and second == real_second and third == real_third:
            conn = sqlite3.connect(checkout_db)
            c = conn.cursor()
            c.execute('''CREATE TABLE IF NOT EXISTS pencil_table (user text, start timestamp);''')
            c.execute('''INSERT into pencil_table VALUES (?,?);''', (user, datetime.datetime.now())) # pencil is now being borrowed
            conn.commit()
            conn.close()
            return "Success!"
