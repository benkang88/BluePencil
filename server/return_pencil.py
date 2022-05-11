import sqlite3
import datetime
user_db = '/var/jail/home/team39/databases/users_locations.db'
pencil_db = '/var/jail/home/team39/databases/borrowed_pencils.db'
user_data = "/var/jail/home/team39/databases/user_data.db"

def request_handler(request):
    if request["method"] == "POST":
        lat = 0
        lon = 0
        try:
            lat = float(request["form"]["lat"])
            lon = float(request["form"]["lon"])
        except Exception as e:
            return "Invalid inputs!"

        conn = sqlite3.connect(user_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS user_table (user text, lat real, lon reals, timing timestamp);''')
        thirty_seconds_ago = datetime.datetime.now() - datetime.timedelta(seconds = 30)
        all_users = c.execute('''SELECT * FROM user_table WHERE timing > ?;''', (thirty_seconds_ago,)).fetchall()
        min_distance = float('inf')
        for user in all_users:
            if distance((lat, lon), (user[1], user[2])) < min_distance:
                min_distance = distance((lat, lon), (user[1], user[2]))
                closest_user = user[0]
        conn.commit()
        conn.close()
        if closest_user is not None:
            conn = sqlite3.connect(pencil_db)
            c = conn.cursor()
            c.execute('''CREATE TABLE IF NOT EXISTS pencils (user text, timing timestamp);''')
            c.execute('''DELETE FROM pencils WHERE user = ?;''', (closest_user,))

            conn.commit()
            conn.close()
            stats(closest_user)
            return "Removed borrow session for " + str(closest_user)
        else:
            return "No closest user"
    elif request["method"] == "GET":
        try:
             user = request["values"]["user"]
        except:
             return "Invalid inputs!"

        conn = sqlite3.connect(pencil_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS pencils (user text, timing timestamp);''')
        active_borrows = c.execute('''SELECT * FROM pencils WHERE user = ?;''', (user,)).fetchall()
        conn.commit()
        conn.close()
        if len(active_borrows) == 0:
            return "NO"
        else:
            return "YES"

def stats(user):
    conn = sqlite3.connect(user_data)
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS user_stats (user text, timing timestamp, checkouts int, last_station text);''')
    try:
        value = c.execute('''SELECT checkouts FROM user_stats WHERE user = ?;''', (user,)).fetchone()
        checkouts = value[0]
        c.execute('''UPDATE user_stats SET checkouts = ? WHERE user = ?;''', (checkouts + 1, user))
    except:
        c.execute('''INSERT into user_stats VALUES (?,?,?,?);''', (user, datetime.datetime.now(),1 ,"NO CHECKOUTS"))
    conn.commit()
    conn.close()
#returns distance in approximate miles between 2 coordinates
def distance(coord1, coord2):
    return 60*((coord1[0] - coord2[0])**2 + (coord1[1] - coord2[1])**2)**0.5
