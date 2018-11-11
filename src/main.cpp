//
//  main.cpp
//  apollo
//
//  Created by Daniel Bogomazov on 2018-06-05.
//  Copyright © 2018 Daniel Bogomazov. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <time.h>
#include <cstdint>
#include <regex>
#include <map>
#include <memory>
#include <sys/stat.h>
#include <chrono>
#include <ctime>

#include <pqxx/pqxx> 

#include "curl_obj.hpp"

#define USE_LOCAL_HTML 0

#define MAX_VALID_YR    9999
#define MIN_VALID_YR    1900

static int currentYear = 1900;
static const std::string months[13] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December", "Unscheduled_and_TBA"};

struct Recording {
    std::string name;
    std::string artist;
    std::string genres;
    std::string labels;
    std::string producers;
    std::string releaseDate;
};

    // Prototype functions
    std::string trimTags(std::string);
    std::string formatDate(std::string, int);
    bool isValidDate(const int&, const int&, const int&);
    std::map<int,size_t> findMonthPositions(const std::string&);
    void updateRecording(pqxx::nontransaction&, const pqxx::result&, const std::shared_ptr<Recording>&);
    void promptForManualEditing(const pqxx::result&, const std::vector<std::shared_ptr<Recording>>&, const std::string&);
    std::vector<int> findMatchingRecordingIDs(pqxx::nontransaction&, const std::vector<std::shared_ptr<Recording>>&);
    std::vector<std::string> findMatchingRecordingNames(pqxx::nontransaction&, const std::vector<std::shared_ptr<Recording>>&);
    std::string addNewArtist(pqxx::nontransaction&, const std::string&);
    std::string addNewProducer(pqxx::nontransaction&, const std::string&);
    std::string addNewLabel(pqxx::nontransaction&, const std::string&);
    std::string addNewGenre(pqxx::nontransaction&, const std::string&, const std::string&);
    std::string addNewRecording(pqxx::nontransaction&, const std::string&, const std::string&);
//    void addNewRecording(pqxx::nontransaction&, const std::shared_ptr<Recording>&);
    void pullFromHTML(const std::string&, std::map<std::string,std::vector<std::shared_ptr<Recording>>>&, std::vector<std::shared_ptr<Recording>>&);
    void updateDB(pqxx::nontransaction&, const  std::map<std::string,std::vector<std::shared_ptr<Recording>>>&, const std::vector<std::shared_ptr<Recording>>&);
    std::string dbToJSON(pqxx::nontransaction&);
    void editStrForJSON(std::string&);


int main() {
    
#if USE_LOCAL_HTML
    std::cout << "\n>>>>> ATTENTION! RUNNING IN DEBUG! <<<<<\n\n";
#endif

    try {
        // Connect to the database
        pqxx::connection connection("dbname = apollo user = apollo hostaddr = 127.0.0.1 port = 5432");

        if (connection.is_open()) {
            std::cout << "\n\n[" << std::time(0) << "] OPENED " << connection.dbname() << "\n";
        } else {
            std::cerr << "Can't connect to the database\n";
            return 1;
        }

        connection.prepare("FindArtistName", "SELECT * FROM artist WHERE name = $1");
        connection.prepare("FindGenreID", "SELECT genre_id FROM genre WHERE name = $1 AND subgenre_name = $2");
//        connection.prepare("FindArtistName", "SELECT * FROM recording WHERE artist = $1");
        connection.prepare("FindRecordingName", "SELECT * FROM recording WHERE name = $1");
        connection.prepare("FindRecordingAndArtist", "SELECT * FROM recording WHERE name = $1 AND artist = $2");

        connection.prepare("AddNewArtist", "INSERT INTO artist (name) VALUES ($1) RETURNING artist_id");
        connection.prepare("AddNewProducer", "INSERT INTO producer (name) VALUES ($1) RETURNING producer_id");
        connection.prepare("AddNewLabel", "INSERT INTO label (name) VALUES ($1) RETURNING label_id");
        connection.prepare("AddNewGenre", "INSERT INTO genre (name, subgenre_name) VALUES ($1, $2) RETURNING genre_id");
        connection.prepare("AddNewRecording", "INSERT INTO recording (name, release_date) VALUES ($1, $2) RETURNING recording_id");

//        connection.prepare("AddNewRecording", "INSERT INTO recording (name, artist, releaseDate, genres, labels, producers) VALUES ($1, $2, $3, $4, $5, $6)");
//        connection.prepare("AddNewRecordingNoDate", "INSERT INTO recording (name, artist, genres, labels, producers) VALUES ($1, $2, $3, $4, $5)");

        connection.prepare("UpdateRecording", "UPDATE recording SET name = $2, genres = $3, labels = $4, producers = $5, releaseDate = $6 WHERE recording_id = $1");

        connection.prepare("SelectAll", "SELECT * FROM recording");

        pqxx::nontransaction nontransaction(connection);        

        // Get current year
        time_t currentTime = time(nullptr);
        struct tm* aTime = localtime(&currentTime);
        currentYear = aTime->tm_year + 1900;

        // String that will hold the raw HTML
        std::string data = "";

#if USE_LOCAL_HTML
        // Read the HTML file in binary to be able to find the size
        std::ifstream htmlFile("../wiki_testing/List of 2018 albums - Wikipedia.html", std::ios::in | std::ios::binary);
        if (htmlFile) {
            // Move to the last binary position in the ifstream and resize the string to the same size
            htmlFile.seekg(0, std::ios::end);
            data.resize(htmlFile.tellg());
            // Move back to the first position in the ifstream and copy the file to the string
            htmlFile.seekg(0, std::ios::beg);
            htmlFile.read(&data[0], data.size());
            htmlFile.close();
        } else {
            std::cerr << "DEBUG ERROR -- Can't open local HTML file\n";
        }
#else
        // Instantiate Curl objectect and pull the HTML
        std::string url = "https://en.wikipedia.org/wiki/List_of_" + std::to_string(currentYear) + "_albums";
        
        CurlObj curl = CurlObj(url);
        data = curl.getData();
#endif
        // Recording storage
        std::map<std::string,std::vector<std::shared_ptr<Recording>>> artistRecordings;
        std::vector<std::shared_ptr<Recording>> recordings;

//        pullFromHTML(data, artistRecordings, recordings);
//        updateDB(nontransaction, artistRecordings, recordings);
        std::cout << addNewRecording(nontransaction, "TestName4", "01-01-01") << "\n";

        // Disconnect from the db
        connection.disconnect();

    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 2;
    }

    return 0;
}       

std::string dbToJSON(pqxx::nontransaction& n) {
    std::string json = "{\"recordings\": [";
    pqxx::result results = n.prepared("SelectAll").exec();
    for (auto r: results) {

        std::string recording[7] = {};

        recording[0] = r[0].as<std::string>(); // recording_id
        recording[1] = !r[1].is_null() ? r[1].as<std::string>() : ""; // name
        recording[2] = !r[2].is_null() ? r[2].as<std::string>() : ""; // artist
        recording[3] = !r[3].is_null() ? r[3].as<std::string>() : ""; // genrees
        recording[4] = !r[4].is_null() ? r[4].as<std::string>() : ""; // labels
        recording[5] = !r[5].is_null() ? r[5].as<std::string>() : ""; // producers
        recording[6] = !r[6].is_null() ? r[6].as<std::string>() : ""; // releeaseDate

        // Make the strings JSON friendly by replacing double quotes and more consistent by removing newline chars
        for (int i = 0; i < 7; i++) {
            recording[i].erase(std::remove(recording[i].begin(), recording[i].end(), '\n'), recording[i].end());
            std::replace(recording[i].begin(), recording[i].end(), '\"', '\'');
        }

        json += "{";
        json += "\"recording_id\": " + recording[0] + ",";
        json += "\"name\": \"" + recording[1] + "\",";
        json += "\"artist\": \"" + recording[2] + "\",";
        json += "\"genres\": \"" + recording[3] + "\",";
        json += "\"labels\": \"" + recording[4] + "\",";
        json += "\"producers\": \"" + recording[5] + "\",";
        json += "\"releaseDate\": \"" + recording[6] + "\"";
        json += "},";

    }

    if (json.back() == ',') json.pop_back();

    json += "]}";

    return json;
}

/**
 Removes HTML tags from a string
 
 @param fullString HTML data string
 @return String without HTML tags
 */
std::string trimTags(std::string fullString) {
    
    while (fullString.find("<") != std::string::npos) {
        size_t start = fullString.find("<");
        size_t end = fullString.find(">") + 1;
        fullString.erase(start, end - start);
    }
    return fullString;
}

/**
 Checks if the passed in date is valid

 @param day Self-explanitory
 @param month Self-explanitory
 @param year Self-explanitory
 @return true if date is valid, false otherwise
*/
bool isValidDate(const int& day, const int& month, const int& year) {

    bool isLeap = (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));

    if (year > MAX_VALID_YR || year < MIN_VALID_YR || (month < 1 || month > 12) || (day < 1 || day > 31) ||
        (month == 2 && ((isLeap && day > 29) || (!isLeap && day > 28))) || 
        ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30)) {
            
            std::cerr << "ERROR in isValidDate function -- invalid date\n";
            return false;
    }

    return true;
}

/**
 Wikipedia stores the date as MMMM dd but PostgreSQL needs it in the yyyy-mm-dd format

 @param dateString Date string found in the Wikipedia HTML
 @param year Year the recording is set to be released on
 @return Reformated date for PostgreSQL or an empty string if the date was not found in the HTML
*/
std::string formatDate(std::string dateString, int year) {

    std::string month = "";
    std::string day = "";

    // Find month
    // TODO : Change this into a regex search -- the for loop is unncessary
    std::string months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

    for (int i = 0; i < 12; i++) {
        if (dateString.find(months[i]) != std::string::npos) {
            i + 1 < 10 ? month = "0" + std::to_string(i + 1) : month = std::to_string(i + 1);
            break;
        }
    }

    // Find day
    // std::smatch smatch;
    // std::regex regex("\\d\\d?$");
    // std::regex_search(dateString, smatch, regex);
    // day = smatch[0];
    std::regex rx("[0-9][0-9]?");
    std::smatch match;
    regex_search(dateString, match, rx);
    day = match[0];

    if (day.size() == 1) {
        day = "0" + day;
    }

    // Make sure the month has been found
    if (month == "") {
        // HTML does not contain a date
        return "";
    } else if (day == "") {
        // HTML contains the month and year but no the date
        // IMPORTANT : The standard we use for this situation is to set the date in the db as ([1999 + YYYY]-MM-01)
        // Example: A recording released on September 2018 without a day will be stored as 4017-09-01
        // This will be handled on the client-side 

        // TODO : Create test cases for saving a recording with an unknown date and then see what happens when the html is updated to include a date
        //        Expected result is for the date to be changed to the correct month, date, and year
        //        Using the above example with the recording's date being updated to 21 the db will be updated as 2018-09-21
        day = "01";
        year = 1999 + year;
    }

    if (!isValidDate(std::stoi(day), std::stoi(month), year)) {
        return "";
    }

    return std::to_string(year) + "-" + month + "-" + day;
}

/**
 Finds the positions of the month headers in the HTML

 @param html constant reference to the html of the Wikipedia page
 @return map of month headers and their position in the HTML string
*/
std::map<int,size_t> findMonthPositions(const std::string& html) {
   
    std::map<int,size_t> positions;
    std::string months[13] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December", "Unscheduled_and_TBA"};

    for (int i = 0; i < 13; i++) {
        size_t pos = html.find("<span class=\"mw-headline\" id=\"" + months[i] + "\">");
        if (pos != std::string::npos) {
            positions[i] = pos;
        }
    }

    return positions;
}

/**
 Updates all records in the database that do no match the HTML

 @param n nontransaction object used to communicate with the database
 @param r a result object the holds a row from the recordings table
 @recording the recording found in the HTML
*/
void updateRecording(pqxx::nontransaction& n, const pqxx::result& r, const std::shared_ptr<Recording>& recording) {
    // TODO : If the cover field is empty and the html provides a new cover, update the db to include it
    std::string sql = "UPDATE recording SET";
    std::vector<std::string> needsUpdating;

    std::string recording_id = r.begin()[0].as<std::string>();
    std::string name = r.begin()[1].is_null() ? "" : r.begin()[1].as<std::string>();
    std::string genres = r.begin()[3].is_null()     ? "" : r.begin()[3].as<std::string>();
    std::string labels = r.begin()[4].is_null() ? "" : r.begin()[4].as<std::string>();
    std::string producers = r.begin()[5].is_null() ? "" : r.begin()[5].as<std::string>();
    std::string releaseDate = r.begin()[6].is_null() ? "" : r.begin()[6].as<std::string>();

    pqxx::prepare::invocation invocation = n.prepared("UpdateRecording");
    invocation(recording_id);

    recording->name.empty() ? invocation() : invocation(recording->name);
    recording->genres.empty() ? invocation() : invocation(recording->genres);
    recording->labels.empty() ? invocation() : invocation(recording->labels);
    recording->producers.empty() ? invocation() : invocation(recording->producers);
    recording->releaseDate.empty() ? invocation() : invocation(recording->releaseDate);

    invocation.exec();
}

/**
 Prompts for a manual update and logs the HTML and DB recordings that have caused the error

 @param r DB records
 @param recordings HTML records
 @param message a message appended to the error message
*/
void promptForManualEditing(const pqxx::result& r, const std::vector<std::shared_ptr<Recording>>& recordings, const std::string& message) {
    std::cerr << "WARNING! -- Found multiple recordings in the HTML of the same artist that do not match the names of the recordings made by the same artist in the DB\n";
    std::cerr << "-- Please manually change the database to reflect the recordings found in the HTML\n";
    std::cerr << "-- Check the log file for the HTML and DB records\n";
    std::cerr << "-- Appended Message: " << message << "\n";

    // TODO : Change the cout to print a log message to a file -- figure out how to make a good log message (include time, etc)
    std::cout << "==== START OF MANUAL PROMPT ====\n\n";
    std::cout << "HTML Recordings :\n";
    for (size_t i = 0; i < recordings.size(); i++) {
        std::cout << i << "\n";
        std::cout << "RECORDING NAME: " << recordings[i]->name << "\n";
        std::cout << "RECORDING ARTIST: " << recordings[i]->artist << "\n";
        std::cout << "RECORDING RELEASE DATE: " << recordings[i]->releaseDate << "\n";
        std::cout << "RECORDING GENRES: " << recordings[i]->genres << "\n";
        std::cout << "RECORDING LABELS: " << recordings[i]->labels << "\n";
        std::cout << "RECORDING PRODUCERS: " << recordings[i]->producers << "\n";
        std::cout << "\n";
    }

    int db_count = 0;
    std::cout << "\nDB Recordings :\n";
    for (auto recording: r) {
        std::cout << ++db_count << "\n";
        std::cout << "RECORDING ID: " << recording[0].as<std::string>() << "\n";
        std::cout << "RECORDING NAME: " << (recording[1].is_null() ? "" : recording[1].as<std::string>()) << "\n";
        std::cout << "RECORDING ARTIST: " << (recording[2].is_null() ? "" : recording[2].as<std::string>()) << "\n";
        std::cout << "RECORDING GENRES: " << (recording[3].is_null() ? "" : recording[3].as<std::string>()) << "\n";
        std::cout << "RECORDING LABELS: " << (recording[4].is_null() ? "" : recording[4].as<std::string>()) << "\n";
        std::cout << "RECORDING PRODUCERS: " << (recording[5].is_null() ? "" : recording[5].as<std::string>()) << "\n";
        std::cout << "RECORDING RELEASE DATE: " << (recording[6].is_null() ? "" : recording[6].as<std::string>()) << "\n";
    }
    std::cout << "\n\n==== END OF MANUAL PROMPT ====\n\n";
}

/**
 Adds a new artist into the DB

 @param n nontransaction object used to communicate with the database
 @param name the name of the new artist
 @return artist_id of the new artist
*/
std::string addNewArtist(pqxx::nontransaction& n, const std::string& name) {
    auto result = n.prepared("AddNewArtist")(name).exec();
    return result.begin()[0].as<std::string>();
}

/**
 Adds a new producer into the DB

 @param n nontransaction object used to communicate with the database
 @param name the name of the new producer
 @return producer_id of the new producer
*/
std::string addNewProducer(pqxx::nontransaction& n, const std::string& name) {
    auto result = n.prepared("AddNewProducer")(name).exec();
    return result.begin()[0].as<std::string>();
}

/**
 Adds a new label into the DB

 @param n nontransaction object used to communicate with the database
 @param name the name of the new label
 @return label_id of the new label
*/
std::string addNewLabel(pqxx::nontransaction& n, const std::string& name) {
    auto result = n.prepared("AddNewLabel")(name).exec();
    return result.begin()[0].as<std::string>();
}

/**
 Adds new genre into the DB

 @param n nontransaction object used to communicate with the database
 @param name the of the new genre
 @param subgenre_name the subgenre name belonging to the new genre
 @return genre_id of the new genre or genre_id of an existing record if the genre + subgenre pair exists
*/
std::string addNewGenre(pqxx::nontransaction& n, const std::string& name, const std::string& subgenre_name) {
    auto existingRecord = n.prepared("FindGenreID")(name)(subgenre_name).exec();
    if (existingRecord.size() > 0) {
        std::cerr << "Genre " << name << " with subgenre " << subgenre_name << " already exists. Duplicate not created. Returned existing genre_id\n";
        return existingRecord.begin()[0].as<std::string>();
    }
    auto result = n.prepared("AddNewGenre")(name)(subgenre_name).exec();
    return result.begin()[0].as<std::string>();
}

/**
 Adds new genre into the DB

 @param n nontransaction object used to communicate with the database
 @param name the of the new recording
 @param date the release date of the new recording
 @return recording_id of the new recording
*/
std::string addNewRecording(pqxx::nontransaction& n, const std::string& name, const std::string& date) {
    auto result = date == "" ? n.prepared("AddNewRecording")(name)().exec() : n.prepared("AddNewRecording")(name)(date).exec();
    return result.begin()[0].as<std::string>();
}

// /**
//  Adds a new recording to the DB

//  @param n nontransaction object used to communicate with the database
//  @recording the recording found in the HTML
// */
// void addNewRecording(pqxx::nontransaction& n, const std::shared_ptr<Recording>& recording) {
//     if (recording->releaseDate != "") {
//         n.prepared("AddNewRecording")(recording->name)(recording->artist)(recording->releaseDate)(recording->genres)(recording->labels)(recording->producers).exec();
//     } else {
//         // If the date is Unscheduled or TBA, do not add it to the db
//         n.prepared("AddNewRecordingNoDate")(recording->name)(recording->artist)(recording->genres)(recording->labels)(recording->producers).exec();
//     }
// }

/**
 Matches the HTML and DB records and finds the IDs of the records successfully matched

 @param n nontransaction object used to communicate with the database
 @param recording the recording found in the HTML
 @return vector of IDs for the records that matched
*/
std::vector<int> findMatchingRecordingIDs(pqxx::nontransaction& n, const std::vector<std::shared_ptr<Recording>>& recordings) {
    std::vector<int> matchingRecordIDs;
    for (auto recording: recordings) {
        pqxx::result recordingFound = n.prepared("FindRecordingAndArtist")(recording->name)(recording->artist).exec();
        if (recordingFound.size() == 1) {
            // Recording found in the DB, store the recording_id
            matchingRecordIDs.push_back(recordingFound.begin()[0].as<int>());
        }
    }
    return matchingRecordIDs;
}

/**
 Matches the HTML and DB records and finds the names of the records successfully matched

 @param n nontransaction object used to communicate with the database
 @param recording the recording found in the HTML
 @return vector of names for the records that matched
*/
std::vector<std::string> findMatchingRecordingNames(pqxx::nontransaction& n, const std::vector<std::shared_ptr<Recording>>& recordings) {
    std::vector<std::string> matchingRecordIDs;
    for (auto recording: recordings) {
        pqxx::result recordingFound = n.prepared("FindRecordingAndArtist")(recording->name)(recording->artist).exec();
        if (recordingFound.size() == 1) {
            // Recording found in the DB, store the names
            matchingRecordIDs.push_back(recording->name);
        }
    }
    return matchingRecordIDs;
}

/**
 Pulls the recording elements from the html and stores the values

 @param data the HTML being pulled
 @param artistRecordings stores the recordings mapped to the artist
 @param recordings a vector of all the recordings found in the HTML
*/
void pullFromHTML(const std::string& data, std::map<std::string,std::vector<std::shared_ptr<Recording>>>& artistRecordings, std::vector<std::shared_ptr<Recording>>& recordings) {
    std::map<int,size_t> monthPositions = findMonthPositions(data);
    std::string startFlag = "<tr>";
    std::string endFlag = "</tr>";
    std::string currentDate = ""; // To be set whithin the loop based on the HTML
    int counter = 0;
    std::smatch smatch;
    std::regex regex("[a-zA-Z]*");

    // Find all HTML chuncks encapsulated in <tr> tags
    for (size_t start = data.find(startFlag, 0); start != std::string::npos; start = data.find(startFlag, start + 1)) {

        size_t end = data.find(endFlag, start);
        std::string fullString = data.substr(start, end + endFlag.size() - start); // From <tr> to </tr>
        
        if (fullString.find("<td>") != std::string::npos) {
            // New Recording
            std::shared_ptr<Recording> newRecording = std::make_shared<Recording>();

            // Do not set the date for Unscheduled_and_TBA recordings
            if (end < monthPositions[12]) {
                newRecording->releaseDate = formatDate(currentDate, currentYear);
            }

            for (size_t index = fullString.find("<td", 0); index != std::string::npos; index = fullString.find("<td", index + 1)) {
                
                size_t subStart = fullString.find("<td", index);
                size_t subEnd = fullString.find("</td>", index) + 5;
                std::string subString = fullString.substr(subStart, subEnd - subStart);

                if (index == fullString.find("<td", 0) && subString.at(subString.find("<td") + 3) == ' ') {
                    // If the first (index first find) td tag is non-default, save its content as the release date
                    // The second part of the conditional can still be true (when recording name is TBA for example)
                    // and therefore, we must check that the index is also at 0 to make sure we're checking
                    // for the date and not for anything else that may have a non-default td tag

                    subString = trimTags(subString);
                    subString.erase(std::remove(subString.begin(), subString.end(), '\n'), subString.end());

                    if (subString == "TBA") {
                        // The recording is set to be released on a certain month but the date is still TBA
                        // Find the month that the recording belongs to and save it
                        std::regex_search(currentDate, smatch, regex);

                        int monthValue = 0;
                        for (; monthValue < 13; monthValue++) {
                            if (smatch[0] == months[monthValue]) {
                                break;
                            }
                        }

                        // TODO : Create test cases for this -- test when TBA is in the middle of the month, at the end of the month, and beginning of the month, end of final month, start of first month (have Jan only have a TBA date)
                        if (monthPositions[monthValue + 1] > end) {
                            // The TBA date is still within the current month
                            newRecording->releaseDate = formatDate(smatch[0], currentYear);
                        } else {
                            // The TBA date belongs to the next month
                            newRecording->releaseDate = formatDate(months[monthValue + 1], currentYear);
                            currentDate = std::to_string(monthValue + 1) + " 1";
                        }
                    } else {
                        currentDate = subString;
                        newRecording->releaseDate = formatDate(currentDate, currentYear);
                    }

                } else {
                    // Place the information into the new recording
                    subString = trimTags(subString);
                    
                    switch (counter) {
                        case 0:
                            newRecording->artist = subString;
                            ++counter;
                            artistRecordings[newRecording->artist].push_back(newRecording);
                            break;
                        case 1:
                            newRecording->name = subString;
                            ++counter;
                            break;
                        case 2:
                            newRecording->genres = subString;
                            ++counter;
                            break;
                        case 3:
                            newRecording->labels = subString;
                            ++counter;
                            break;
                        case 4:
                            newRecording->producers = subString;
                            ++counter;
                            break;
                        default:
                            // This is the reference in the HTML, don't include this part
                            recordings.push_back(newRecording);
                            counter = 0;
                            break;
                    }
                }
            }
        }
    }
}

/**
 Updates the DB to reflect the HTML data

 @param nontransaction object used to communicate with the database
 @param artistRecordings stores the recordings mapped to the artist
 @param recordings a vector of all the recordings found in the HTML
*/
void updateDB(pqxx::nontransaction& nontransaction, const std::map<std::string,std::vector<std::shared_ptr<Recording>>>& artistRecordings, const std::vector<std::shared_ptr<Recording>>& recordings) {
    // Search through the database to see if the recordings need to be added or modified
    for (auto recording: recordings) {
        // Search if the artist is already in the database
        pqxx::result foundArtists = nontransaction.prepared("FindArtistName")(recording->artist).exec();

        if (foundArtists.size() == 0) {
            // Recording not in the database -- add it
//            addNewRecording(nontransaction, recording);
        } else {
            // There is already a record with this artist in the database
            // Check to see if there is anything that needs updating or if a prompt for a manual edit is required according to the rules below
            //
            //
            // Test Cases: (R# == Recording# ; ? == TBA)
            //          
            //          HTML            DB          Solution
            //
            //          ?               ?           Update [? -> ?]
            //          ?               R1          Update [R1 -> ?]
            //          R1              R1          Update [R1 -> R1]
            //          R1              ?           Update [? -> R1]
            //          
            //          R1,R2           R1,R2       Update [R1 -> R1, R2 -> R2]
            //          R1,?            R1,R2       Update [R1 -> R1, R2 -> ?]
            //          R1,R2           R1,?        Update [R1 -> R1, ? -> R2]
            //          R1,?            R1,?        Update [R1 -> R1, ? -> ?]
            //          R1,R2           R1,R3       Update [R1 -> R1, R2 -> R3]
            //          R1,R2           R3,R4       Prompt (Edit)
            //          ?,?             R1,?        Prompt (Edit) 
            //          ?,?             ?,?         Prompt (Edit)
            //          R1,?            ?,?         Prompt (Edit)
            //          
            //          R1,R2,R3        R1,R2       Add [R3], Update [R1 -> R1, R2 -> R2]
            //          R1,R2,R3        R1,R4       Prompt (Add and Edit), Update [R1 -> R1]
            //          R1,R2,?         R1,R2       Add [?], Update [R1 -> R1, R2 -> R2]
            //          R1,R2,?         ?,?         Prompt (Add and Edit)
            //          R1,R2,?         R1,?        Prompt (Add and Edit), Update [R1 -> R1]
            //          R1,?,?          R1,?        Prompt (Add and Edit), Update [R1 -> R1] 
            //          R1,?,?          R1,R2       Prompt (Add and Edit), Update [R1 -> R1]
            //
            //          TODO : Figure out WHEN to create delete prompts -- will have to compare DB to HTML - instead of HTML to DB 
            //                 -- Figure out which artists in the DB have less records than their HTML counterparts
            //                 -- If a DB entry has 1 recording but the HTML doesn't have that record - how would it even check that record?
            //
            //          R1              R1,R2       Prompt (Delete), Update [R1 -> R1]
            //          R1              R2,R3       Prompt (Delete and Edit)
            //          ?               ?,?         Prompt (Delete and Edit)
            //          ?               R1,?        Prompt (Delete), Update [? -> ?]
            //
            pqxx::result foundRecording = nontransaction.prepared("FindRecordingAndArtist")(recording->name)(recording->artist).exec();

            bool multipleTBA = false;
            if (recording->name == "TBA") {
                // Check to see if the artist only has one TBA recording in the HTML
                // If there is more than one, we should note it in case of something like the following example:
                // 
                //  HTML: ?,?  --- DB: R1,?
                // If we don't note that HTML has 2+ TBA recordings, it will end up updating the one TBA recording in the DB twice
                int count = 0;
                for (auto recording: artistRecordings.at(recording->artist)) {
                    if (recording->name == "TBA" && ++count > 1) {
                        multipleTBA = true;
                        break;
                    }
                }
            }
            if (foundRecording.size() > 1) {
                promptForManualEditing(foundArtists, artistRecordings.at(recording->artist), "Found more than 1 records with the same artist AND recording name\n-- In theory, an artist shouldn't have multiple recordings of the same name but this one did -- resolve issue manually\n");
            } else if (foundRecording.size() == 1 && !multipleTBA) {
                updateRecording(nontransaction, foundRecording, recording);
            } else {
                // Since we know that the current artist is in the database but the recording name is not found, check to see if there are the same
                // number of records in the HTML as there are in the DB. 
                //
                // If the HTML and DB have the same number of records, check to see if the other HTML records match those in the DB. Store the recording_id's of all that do.
                //   If all the HTML records match the DB records, update DB record whose id wasn't stored with the current HTML recording
                //   Else if one or more (excluding the current recording) do not match, prompt for manual editing
                //
                // Else if the HTML has more records, check to see if the other HTML records match those in the DB. Store the recording_id's of all that do.
                //   If all the other HTML records match the DB records, append the new recording to the DB.
                //   Else if one or more (excluding the current recording) do not match, do not edit -- prompt for manual update. 
                //
                // Else, if the DB has more records than the HTML -- something was deleted -- prompt for manual update

                if (artistRecordings.at(recording->artist).size() == foundArtists.size()) {
                    std::vector<int> matchingRecordIDs = findMatchingRecordingIDs(nontransaction, artistRecordings.at(recording->artist));

                    if (matchingRecordIDs.size() == artistRecordings.at(recording->artist).size() - 1) {
                        std::string sql = "SELECT * FROM recording WHERE artist = $1";
                        size_t i;
                        for (i = 0; i < matchingRecordIDs.size(); i++) {
                            sql += " AND NOT recording_id = $" + std::to_string(i+2);
                        }

                        // TODO : The below prepare seems a little bit hackish, try to change it
                        //         -- The reason I did it this way is because if there are multiple instances where a record is found under these conditions,
                        //          -- if the multiple times this code is ran as just "FindRecordingAndArtist_FilteringRecordingIDs" without the added number,
                        //           -- pqxx will complain that I'm redefining "FindRecordingAndArtist_FilteringRecordingIDs" with more/less invocations
                        //            -- ex: first time, it finds 3 IDs, second time it finds 2 
                        //             -- one requires invocation(a)(id)(id)(id), the other invocation (a)(id)(id)
                        nontransaction.conn().prepare("FindRecordingAndArtist_FilteringRecordingIDs" + std::to_string(i), sql);
                        
                        // Setup dynamic parameters for the query
                        pqxx::prepare::invocation invocation = nontransaction.prepared("FindRecordingAndArtist_FilteringRecordingIDs" + std::to_string(i));
                        invocation(recording->artist);
                        for (auto id: matchingRecordIDs) {
                            invocation(id);
                        }
                        // Update the missnamed recording
                        pqxx::result missnamedRecording = invocation.exec();
                        updateRecording(nontransaction, missnamedRecording, recording);
                    } else {
                        promptForManualEditing(foundArtists, artistRecordings.at(recording->artist), "Artist # are the same in the DB and HTML but there are 2+ recording names that differ in the HTML vs the DB.");
                    }
                } else if (artistRecordings.at(recording->artist).size() > foundArtists.size()) {
                    std::vector<std::string> matchingRecordNames = findMatchingRecordingNames(nontransaction, artistRecordings.at(recording->artist));

                    if (matchingRecordNames.size() == foundArtists.size() &&
                        std::find(matchingRecordNames.begin(), matchingRecordNames.end(), recording->name) == matchingRecordNames.end()) {
                        // All of the DB records have been found in the HTML but the current record has not -- append
//                        addNewRecording(nontransaction, recording);
                    } else if (matchingRecordNames.size() < artistRecordings.at(recording->artist).size() - 1) {
                        promptForManualEditing(foundArtists, artistRecordings.at(recording->artist), "HTML has more Artists than the DB so we should append but there are 2+ records in the HTML that do not match the DB -- one could be an update and the other could be an append.");
                    } else if (multipleTBA) {
                        promptForManualEditing(foundArtists, artistRecordings.at(recording->artist), "Add and Edit");
                    } else {
                        std::cerr << "ERROR in main function -- something is wrong with how we treat comparing the HTML and DB records for adding/editing/deleting\n";
                    }
                }
            } 
        }
    }

    // Save database as a JSON file and create archive
    std::string json = dbToJSON(nontransaction);
    std::string filepath = "/data/imm/json/recordings/";
    std::ofstream fs(filepath + "current.json");
    if (fs) {
        fs << json;
        auto epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); // epoch time with milliseconds
        fs = std::ofstream(filepath + "archive/" + std::to_string(epoch) + ".json");
        if (fs) {
            fs << json;
            fs.close();
        } else {
            std::cerr << "ERROR in updateDB -- Couldn't create recordings archive file\n-- Tried to create file: " << std::to_string(epoch) + ".json\n";
        }
    } else {
        std::cerr << "ERROR in updateDB -- Couldn't write to file recordings/current.json\n";
    }

    nontransaction.commit();
}
