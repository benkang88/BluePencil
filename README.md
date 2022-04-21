# BluePencils

## Week 2 Writeup

### Server side code

#### Databases
The server-side code is broken up into four databases and three endpoints. The databases are: a stations database (storing the currently existing stations, their locations, and the number of pencils they currently have), a user data database (storing usernames and their corresponding passwords for access into the BluePencils system), a codes database (storing the associated station and user as well as the correct code and the time of issue, used for storing temporary codes that allow users to unlock pencils from stations), and a borrowed pencils database (storing the user and the associated checkout timestamp, used to keep track of each checked out pencil each user has, along with its associated checkout time). These databases store information that the backend will use to determine what stations to send to user clients, and also to check code correctness with station clients.

#### Adding and Updating Stations
The backend code that controls adding stations and updating station locations and pencil counts is aptly called stations.py. 