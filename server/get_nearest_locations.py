import sqlite3
location_db = '/var/jail/home/team39/databases/location.db'

def request_handler(request):
    #question, what does post do? 
    if request["method"] == "POST":
        lat = 0
        lon = 0
        station_name = ""
        try:
            station_name = request['form']['station']
            lat = float(request['form']['lat'])
            lon = float(request['form']['lon'])
        except Exception as e:
            return "Invalid inputs!"

        conn = sqlite3.connect(location_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS station_table (station text, lat real, lon reals);''')

        if c.execute('''SELECT station from station_table WHERE station = ?;''', (station_name,)).fetchone() is None:
            c.execute('''INSERT into station_table VALUES (?,?,?);''', (station_name, lat, lon))
        else:
            c.execute('''UPDATE station_table SET lat = ?, lon = ? WHERE station = ?;''', (lat, lon, station_name))

        conn.commit()
        conn.close()
        #things = c.execute('''SELECT * from station_table WHERE station = ?;''', (station_name,)).fetchone()
        #return things
    else:
        lat = 0
        lon = 0
        radius = 0
        try:
            lat = float(request["values"]["lat"])
            lon = float(request["values"]["lon"])
            radius = float(request["values"]["radius"])
        except Exception as e:
            return "Invalid inputs!"

        conn = sqlite3.connect(location_db)
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS station_table (station text, lat real, lon reals);''')

        all_stations = c.execute('''SELECT * FROM station_table;''').fetchall()
        locations = {}
        for station in all_stations:
            locations[station[0]] = (station[1], station[2])
        closest_locations = getLocations((lat, lon), locations, radius)

        conn.commit()
        conn.close()
        return closest_locations

#returns distance in approximate miles between 2 coordinates
def distance(coord1, coord2):
    return 60*((coord1[0] - coord2[0])**2 + (coord1[1] - coord2[1])**2)**0.5

def getLocations(coord, locations, radius):

    closestLocations = []
    distances = []
    for loc in locations:
         if distance(coord, locations[loc]) < radius:
            closestLocations.append(loc)
            distances.append(distance(coord, locations[loc]))
    sortedIndices = [i[0] for i in sorted(enumerate(distances), key=lambda x:x[1])]
    return [(closestLocations[i], distances[i]) for i in sortedIndices]

def sign(x):
    if x > 0:
        return 1
    elif x == 0:
        return 0
    else:
        return -1

def within_area(point_coord,poly):
    pass #Your code here

    count = 0

    for i in range(len(poly)):
        p1x = poly[i-1][0] - point_coord[0]
        p1y = poly[i-1][1] - point_coord[1]

        p2x = poly[i][0] - point_coord[0]
        p2y = poly[i][1] - point_coord[1]

        if sign(p2y) != sign(p1y):
            if sign(p1x) >= 0 and sign(p2x) >= 0:
                count += 1
            elif sign(p1x) != sign(p2x):
                intersect = (p1x*p2y - p1y*p2x)/(p2y-p1y)
                if intersect >= 0:
                    count += 1
    return count % 2 == 1

def get_area(point_coord,locations):
    for location in locations:
        if within_area(point_coord, locations[location]):
            return location

    return "Off Campus"
    pass #Your code here
