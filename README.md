# apollo-backend
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
