#include "SessionStore.hpp"

#include <sstream>

SessionStore::SessionStore() : _next_id(1) {}

SessionStore::~SessionStore()
{
    for (std::map<std::string, Session *>::iterator it = _sessions.begin();
         it != _sessions.end(); ++it)
        delete it->second;
}

SessionStore &SessionStore::instance()
{
    static SessionStore store;
    return store;
}

Session *SessionStore::create()
{
    std::ostringstream oss;
    oss << _next_id++ << "_" << std::time(0);
    std::string sid = oss.str();
    Session *s = new Session(sid);
    _sessions[sid] = s;
    return s;
}

Session *SessionStore::get(const std::string &id)
{
    std::map<std::string, Session *>::iterator it = _sessions.find(id);
    if (it == _sessions.end())
        return NULL;
    if (it->second->expired())
    {
        destroy(id);
        return NULL;
    }
    return it->second;
}

void SessionStore::destroy(const std::string &id)
{
    std::map<std::string, Session *>::iterator it = _sessions.find(id);
    if (it == _sessions.end())
        return;
    delete it->second;
    _sessions.erase(it);
}

void SessionStore::cleanup()
{
    std::map<std::string, Session *>::iterator it = _sessions.begin();
    while (it != _sessions.end())
    {
        if (it->second->expired())
        {
            delete it->second;
            _sessions.erase(it++);
        }
        else
            ++it;
    }
}
