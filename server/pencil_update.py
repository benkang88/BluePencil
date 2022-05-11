import sqlite3

pencil_db = '/var/jail/home/team39/databases/pencil.db'

def request_handler(request):
   if request["method"] == "GET":
       station_name = ""
       try:
          station_name = request["values"]["station"]
       except:
          return "Invalid inputs!"
       
       conn = sqlite3.connect(pencil_db)
       c = conn.cursor()
       c.execute('''CREATE TABLE IF NOT EXISTS pencil_amounts (station text, num_pencils real);''')
       request = c.execute('''SELECT num_pencils FROM pencil_amounts WHERE station = ?;''', (station_name,)).fetchone()
       conn.commit()
       conn.close()
       if not request:
          return "No station with given name " + station_name + " found"
       else:
          return int(request[0])
   elif request["method"] == "POST":
        station_name = ""
        try:
          station_name = request["form"]["station"]
          num_pencils = int(request["form"]["num_pencils"])
        except:
          return "Invalid inputs!"
        conn = sqlite3.connect(pencil_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS pencil_amounts (station text, num_pencils real);''')
        request = c.execute('''SELECT num_pencils FROM pencil_amounts WHERE station = ?;''', (station_name,)).fetchone()
        if request:
             curr_pencils = int(request[0])
             c.execute(''' UPDATE pencil_amounts SET num_pencils = ? WHERE station = ?;''', (curr_pencils + num_pencils, station_name))
             conn.commit()
             conn.close()
             return "Pencil amount sucessfully updated! Current number of pencils: " + str(curr_pencils + num_pencils)
        else:
             c.execute(''' INSERT INTO pencil_amounts (station, num_pencils) VALUES (?,?); ''', (station_name, num_pencils))
             conn.commit()
             conn.close()
             return "New station " + station_name + " created with  " + str(num_pencils) + " pencils" 
