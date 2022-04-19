from datetime import datetime
import sqlite3
import random
pencil_data = "/var/jail/home/team39/borrowed_pencils.db"
code_data = "/var/jail/home/team39/codes.db"


def request_handler(request):
    create_code_database()
    create_pencil_database()
    if request["method"] == "POST":
        try:
            user = request['form']['username']
            # insert_into_database(user)
            return create_code(user)
        except Exception as e:
            return "Error: username is missing"
    else:
        try:
            code = request['values']['code']
            user = search_code(code)
            if user is not None:
                insert_into_pencil_database(user)
                return "Unlocked!"
            return "incorrect code"
        except Exception as e:
            return "Error: username is missing"

def create_pencil_database():
    conn = sqlite3.connect(pencil_data)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS pencil_data (user text, start timestamp);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database

def create_code_database():
    conn = sqlite3.connect(code_data)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS code_data (user text, code text, start timestamp);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database

def insert_into_pencil_database(user):
    conn = sqlite3.connect(pencil_data)
    c = conn.cursor()
    c.execute('''INSERT into pencil_data VALUES (?,?);''', (user, datetime.now()))
    conn.commit()
    conn.close()

def create_code(user):
    code = ""
    for i in range(5):
        code += str(random.randint(1,3))
    conn = sqlite3.connect(pencil_data)
    c = conn.cursor()
    c.execute('''INSERT into code_data VALUES (?,?);''', (user, code, datetime.now()))
    conn.commit()
    conn.close()
    return code

def search_code(code):
    code = ""
    for i in range(5):
        code += str(random.randint(1,3))
    conn = sqlite3.connect(pencil_data)
    c = conn.cursor()
    thirty_seconds_ago = datetime.datetime.now()- datetime.timedelta(seconds = 30) # create time for fifteen minutes ago!
    user = c.execute('''SELECT user FROM code_data WHERE code = ? AND start > ?;''', (code, thirty_seconds_ago)).fetchone()
    conn.commit()
    conn.close()
    return user[0]
