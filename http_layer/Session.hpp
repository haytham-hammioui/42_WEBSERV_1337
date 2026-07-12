#pragma once

#include <ctime>
#include <map>
#include <string>

struct Session
{
    std::string id;
    std::map<std::string, std::string> data;
    std::time_t created_at;
    int max_age;

    Session(const std::string &sid);
    bool expired() const;
};
