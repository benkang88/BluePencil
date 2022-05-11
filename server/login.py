import sqlite3
import datetime
user_data = "/var/jail/home/team39/databases/user_data.db"

def request_handler(request):
    create_database()
    if request["method"] == "POST":
        try:
            user = request['form']['username']
            pw = request['form']['password']
            return login(user, pw)
        except Exception as e:
            return "Error: username or password is missing"
    else:
        return "Only Post requests allowed"

def create_database():
    conn = sqlite3.connect(user_data)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS user_data (user text, pw text);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database

def insert_into_database(user, pw):
    conn = sqlite3.connect(user_data)
    c = conn.cursor()
    c.execute('''INSERT into user_data VALUES (?,?);''', (user, pw))
    conn.commit()
    conn.close()

def create_stats(user):
    conn = sqlite3.connect(user_data)
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS user_stats (user text, timing timestamp, checkouts int, last_station text);''')
    c.execute('''INSERT into user_stats VALUES (?,?,?,?);''', (user, datetime.datetime.now(), 0, "Not available"))
    conn.commit()
    conn.close()
 
def login(user, pw):
    conn = sqlite3.connect(user_data)
    c = conn.cursor()
    correct_pw = c.execute('''SELECT pw FROM user_data WHERE user = ?;''',(user,)).fetchone()
    if correct_pw is None:
        insert_into_database(user, pw)
        create_stats(user)
        return "Welcome to BluePencils, " + user + "!"
    conn.commit()
    conn.close()
    if pw == correct_pw[0]:
        return "Welcome back, " + user + "!"
    else:
        return "Incorrect password!"
