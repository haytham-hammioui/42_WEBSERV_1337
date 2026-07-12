#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include "Config.hpp"

struct CgiResult {
    int         status_code;
    std::string headers;   
    std::string body;      
    bool        timed_out; 
    bool        error;     

    CgiResult() : status_code(200), timed_out(false), error(false) {}
};

class CgiHandler {
public:
    CgiHandler();
    CgiHandler(const CgiHandler& other);
    CgiHandler& operator=(const CgiHandler& other);
    ~CgiHandler();







    CgiResult execute(const std::string&                    interpreter,
                      const std::string&                    script_path,
                      const std::string&                    request_body,
                      const std::map<std::string,std::string>& env_vars,
                      int                                   timeout_sec);

private:

    char**      buildEnv(const std::map<std::string,std::string>& env_vars);
    void        freeEnv(char** env, size_t count);


    void        parseCgiOutput(const std::string& raw,
                               std::string&       headers,
                               std::string&       body);
};

#endif