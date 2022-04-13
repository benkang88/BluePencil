import sqlite3
user_data = "/var/jail/home/team39/user_data.db" # just come up with name of database


def request_handler(request):
    create_database()
    if request["method"] == "POST":
        try:
            user = request['values']['username']
            pw = request['values']['password']
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

def login(user, pw):
    conn = sqlite3.connect(user_data)
    c = conn.cursor()
    correct_pw = c.execute('''SELECT pw FROM user_data WHERE user = ?;''',(user,)).fetchone()
    if correct_pw is None:
        insert_into_database(user, pw)
        return "Welcome to BluePencils, " + user + "!"
    conn.commit()
    conn.close()
    if pw == correct_pw:
        return "Welcome back, " + user + "!"
    else:
        return "Incorrect password!"