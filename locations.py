locations={
    "Student Center":[(-71.095863,42.357307),(-71.097730,42.359075),(-71.095102,42.360295),(-71.093900,42.359340),(-71.093289,42.358306)],
    "Dorm Row":[(-71.093117,42.358147),(-71.092559,42.357069),(-71.102987,42.353866),(-71.106292,42.353517)],
    "Simmons/Briggs":[(-71.097859,42.359035),(-71.095928,42.357243),(-71.106356,42.353580),(-71.108159,42.354468)],
    "Boston FSILG (West)":[(-71.124664,42.353342),(-71.125737,42.344906),(-71.092478,42.348014),(-71.092607,42.350266)],
    "Boston FSILG (East)":[(-71.092409,42.351392),(-71.090842,42.343589),(-71.080478,42.350900),(-71.081766,42.353771)],
    "Stata/North Court":[(-71.091636,42.361802),(-71.090950,42.360811),(-71.088353,42.361112),(-71.088267,42.362476),(-71.089769,42.362618)],
    "East Campus":[(-71.089426,42.358306),(-71.090885,42.360716),(-71.088310,42.361017),(-71.087130,42.359162)],
    "Vassar Academic Buildings":[(-71.094973,42.360359),(-71.091776,42.361770),(-71.090928,42.360636),(-71.094040,42.359574)],
    "Infinite Corridor/Killian":[(-71.093932,42.359542),(-71.092259,42.357180),(-71.089619,42.358274),(-71.090928,42.360541)],
    "Kendall Square":[(-71.088117,42.364188),(-71.088225,42.361112),(-71.082774,42.362032)],
    "Sloan/Media Lab":[(-71.088203,42.361017),(-71.087044,42.359178),(-71.080071,42.361619),(-71.082796,42.361905)],
    "North Campus":[(-71.11022,42.355325),(-71.101280,42.363934),(-71.089950,42.362666),(-71.108361,42.354484)],
    "Technology Square":[(-71.093610,42.363157),(-71.092130,42.365837),(-71.088182,42.364188),(-71.088267,42.362650)]
}

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

def request_handler(request):
    if request["method"] == "POST":
        return "POST requests not allowed."
    if 'lat' not in request['args'] or 'lon' not in request['args']:
        return "You must enter a lat and lon value."
    try:
        lat = float(request["values"]["lat"])
        lon = float(request["values"]["lon"])
    except:
        return "Both lat and lon must be valid numbers."
    return get_area((lon, lat), locations)


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