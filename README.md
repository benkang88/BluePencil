# BluePencils

## Week 2 Writeup

### Server side code

#### Databases
The server-side code is broken up into four databases and three endpoints. The databases are: a stations database (storing the currently existing stations, their locations, and the number of pencils they currently have), a user data database (storing usernames and their corresponding passwords for access into the BluePencils system), a codes database (storing the associated station and user as well as the correct code and the time of issue, used for storing temporary codes that allow users to unlock pencils from stations), and a borrowed pencils database (storing the user and the associated checkout timestamp, used to keep track of each checked out pencil each user has, along with its associated checkout time). These databases store information that the backend will use to determine what stations to send to user clients, and also to check code correctness with station clients.

#### Adding and Updating Stations
The backend code that controls adding and updating stations is aptly called `station.py`. It handles queries from new stations wanting to be added to the `station.db` database. If the station POSTing to the backend already exists in the database, then the code will instead treat the POST as an update to the existing entry in the table, updating the station's latitude and longitude as well as the current number of pencils in the station.

#### User Login
The `login.py` file handles user login. Upon a user client submitting a POST request containing its username and (future to be encrypted) password, the file will match the username to the password in the `user_data.db` database and, upon finding a match, return a statement telling the user that they hae successfully logged in. When a match is not found, the file will tell the user client that the username and password do not match, upon which the user client will prompt the user to try again.

#### Creating and Testing Codes
The primary mechanism of our system is to have user clients detect nearby stations and then attempt to check out a pencil from that station. Upon sending this request, the user will input a code sent onto their user ESP into the station ESP. The structure of this interaction is as follows: the user client will send a POST request to the `get_code.py` file indicating that the user wishes to check out a pencil from a chosen station. The file will then create a code, input the code, user, station, and timestamp into the `codes.db` database, and send the code back to the user client. The user will then have 30 seconds to input the correct code onto the station client and POST the code to the `code_checker.py` file. This file will query the `codes.db` database and check for a matching station-code pair, and if found, find the associated user and timestamp. If the user inputted the code in time, `code_checker.py` will update the `borrowed_pencils.db` database with the user and current timestamp of the checked out pencil and return a message to the station telling it to dispense a pencil for the user.

### Client side code

#### User client
The `user.ino` file contains the code that will be run on every user ESP. The general structure of the user is that it stores two states, a `login_status` and a `system_status`. The `login_status` state is useful only when booting the user ESP; it is used to allow the user to input their username and password in order to log into their BluePencils account. Upon successful login, the `system_status` will transfer from the LOGIN status, which is used to allow the user to log in, to the STATION_FINDER status, the primary state of `system_status`. In this state, the user client will be able to find nearby stations and choose one of them to check out a pencil from. The final `system_status` state is USER_STATS, where pushing a button allows the user to view their stats, such as the log of pencils they have checked out in their lifetime as well as the status of current pencils that they have checked out.

#### Server client (to be completed next week)
The server client is responsible for controlling the code input as well as the mechanical dispensal of pencils. The relevant code for this section will be a deliverable for next week, and will interact with the rest of the code as described above.