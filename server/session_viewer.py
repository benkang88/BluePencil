import sqlite3
import datetime
pencil_db = '/var/jail/home/team39/databases/borrowed_pencils.db'

def request_handler(request):
  conn = sqlite3.connect(pencil_db)
  c = conn.cursor()
  c.execute('''CREATE TABLE IF NOT EXISTS pencils (user text, timing timestamp);''')
  active_borrows = c.execute('''SELECT * FROM pencils;''').fetchall()
  conn.commit()
  conn.close()
  return active_borrows

