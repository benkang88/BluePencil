import sqlite3
import datetime
user_data = "/var/jail/home/team39/databases/user_data.db"

def request_handler(request):
    create_database()
    if request["method"] == "GET":
        user = ""
        try:
            user = request['values']['user']
        except Exception as e:
            return "Error: user field is missing"
        stats = get_stats(user)
        name = stats[0]
        timediff = (datetime.datetime.now() - datetime.datetime.strptime(stats[1], '%Y-%m-%d %H:%M:%S.%f')).total_seconds()
        days = int(timediff//(60*60*24))
        hours = int((timediff % (60*60*24))//(60*60))
        checkouts = stats[2]
        last_station = stats[3]

        
        return format_response(user, days, hours, checkouts, last_station)
    else:
        return "Only Get requests allowed"


def create_database():
    conn = sqlite3.connect(user_data)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS user_stats (user text, timing timestamp, checkouts int, last_station text);''')
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database


def get_stats(user):
    conn = sqlite3.connect(user_data)
    c = conn.cursor()
    stats = c.execute('''SELECT *  FROM user_stats WHERE user = ?;''',(user,)).fetchone()
    conn.commit()
    conn.close()
    return stats

def format_response(user, days, hours, checkouts, last_station):
    return r'''
<h1> {}'s Statistics</h1>
<p> Lifetime on BluePencils: {} days, {} hours </p>
<p> Lifetime checkouts: {} pencils </p>
<p> Last station used: {} </p>
'''.format(user, days, hours, checkouts, last_station)
