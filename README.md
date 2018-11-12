# apollo
A webscraper written in C++ using the cURL library. Scrapes and stores information about upcoming music albums.

## Version
The codebase is in a pre-alpha stage so there is a lot wrong with it. Trying to get it to work first before fixing it up to follow proper coding conventions. Everything is currently using the global namespace - this will change after I figure out how I want everything to be structured after making a working PoC. Might switch it to Python in the future instead of C++.

## Prerequisites
- cURL
- PostgreSQL

## Building [OSX/Linux]
- Allow the setup script to execute
  - `chmod u+x setup.sh`
- Run PostgreSQL
  - `postgres -D /usr/local/opt/pgsql/data`
- Run the setup script
  - `./setup.sh`
- Run CMake
  - `cmake .`
- Run Make
  - `make`
- Running the executable
  - `./apollo`

## Contribution Note
- If the HTML contains the month and year but not the day for the release date of the recording, we set the day to 01. To create a distinction between these dates and release dates _actually_ released on the first of the month, we use the following standard:

  - **IMPORTANT** : The standard we use for this situation is to set the date in the db as ([1999 + YYYY]-MM-01)
    - Example: A recording released on September 2018 without a day will be stored as 4017-09-01
    - Conversion back will be handled on the client-side 
- If you don't have permissions to write to `/data/imm/json/recordings/`, you will recieve an error message. This shouldn't matter as of now as it doesn't block any other functionalities and will only be used later by the front-end. It is used to store an archive of the database. If you want this functionality, simply run the executable as `sudo ./apollo`.
