#pragma once

#include <map>
#include <string>

#include "Session.hpp"

class SessionStore
{
private:
    std::map<std::string, Session *> _sessions;
    unsigned long _next_id;

    SessionStore();
    ~SessionStore();
    SessionStore(const SessionStore &);
    SessionStore &operator=(const SessionStore &);

public:
    static SessionStore &instance();
    Session *create();
    Session *get(const std::string &id);
    void destroy(const std::string &id);
    void cleanup();
};
