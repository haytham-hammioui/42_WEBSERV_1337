#ifndef CGIMANAGER_HPP
#define CGIMANAGER_HPP

#include <map>
#include <vector>
#include <string>
#include <ctime>
#include <sys/types.h>

struct CgiProcess {
    int clientFd;
    pid_t pid;
    int pipeFd;
    std::string output;
    time_t startTime;
};

class CgiManager {
private:
    std::map<int, CgiProcess> _procs;

public:
    void add(int clientFd, pid_t pid, int pipeFd);
    ssize_t readOutput(int pipeFd);
    std::string moveOutput(int pipeFd);
    int getClientFd(int pipeFd) const;
    pid_t getPid(int pipeFd) const;
    bool hasProcess(int pipeFd) const;
    std::vector<int> checkTimeouts(time_t now, int timeoutSec);
    void remove(int pipeFd);
    void killAll();
    ~CgiManager();
};

#endif
