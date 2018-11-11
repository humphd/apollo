#!/bin/bash

## Set up the apollo database using postgres

# check if postgres is installed
which -s psql
if [ ! $? -eq 0 ]
then
	echo "ERROR - postgres not installed"
	exit 1
fi

# check if role exists
# if not - create it
psql -qt -c "SELECT * FROM pg_roles WHERE rolname='apollo'" | cut -d \| -f 1 | grep -qw apollo
if [ ! $? -eq 0 ]
then
	createuser "apollo" -sd
fi

# check if apollo db exists
# if not - create it
psql -lqt | cut -d \| -f 1 | grep -qw apollo
if [ ! $? -eq 0 ]
then
	createdb -O "apollo" -U "apollo" -E "UTF8" "apollo"
fi

# check if tables exist
# if not - create them
psql -q -U "apollo" -d "apollo" -c "SELECT * FROM pg_tables WHERE tablename = 'artist' AND tableowner = 'apollo';" |  grep -qw "(0 rows)"
if [ $? -eq 0 ]
then
	psql -qt -U "apollo" -d "apollo" < create_db.sql
fi
