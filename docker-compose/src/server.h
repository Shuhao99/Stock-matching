#ifndef SERVER_H
#define SERVER_H
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <time.h> 
#include <iostream>
#include <sstream>

#include <fstream>
#include <map>
#include <unordered_map>
#include <vector> 

#include <string>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h> 

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>


#include "socket.h"
#include "session.h"
#include "xml_parser.h"
#include "xml_generator.h"
#include "database.h"


class server {
private:
    static const int MAX_BUFFER_SIZE = 65536;
    static const int BUFFER_SIZE = 4096;
    const char * lisn_port;
    int connection_lisn_fd;
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;

public:
    server(const char * port, int thread_num);
    ~server();

    template<typename... Args>
    void AddTask(void(*function)(Args...), Args... args);
    
    void run();

    int create_session(const int &listener_fd, std::string * ip);
    
    static void handle(session curr_session);

    static string get_xml(const session* curr_session);

    static void handle_symbol(database* const db, const create req, std::vector<create> *responses);

    static void handle_create(database* const db, const create req, std::vector<create> *responses);

    static string handle_tranxt(database* const db, string xml_msg);
};

#endif

