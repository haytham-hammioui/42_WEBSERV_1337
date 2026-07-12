#include "Session.hpp"

Session::Session(const std::string &sid)
    : id(sid), created_at(std::time(0)), max_age(3600) {}

bool Session::expired() const
{
    std::time_t now = std::time(0);
    return (now - created_at) >= max_age;
}
