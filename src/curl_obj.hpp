//
//  curl_obj.hpp
//  apollo
//
//  Created by Daniel Bogomazov on 2018-06-09.
//  Copyright © 2018 Daniel Bogomazov. All rights reserved.
//

#ifndef CurlObj_hpp
#define CurlObj_hpp

#include <curl/curl.h>
#include <string>

class CurlObj {
public:
    /**
     * @brief Constructor for a curl object.
     *
     * @param url The url of the page to scrape.
     */
    CurlObj (std::string url);
    static int curlWriter(char *data, int size, int nmemb, std::string *buffer);
    std::string getData();
protected:
    CURL * curl;
    std::string curlBuffer;
};


#endif /* CurlObj_hpp */
