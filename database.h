#ifndef DATABASE_H
#define DATABASE_H
#include <iostream>
#include <pqxx/pqxx>
#include <vector>
#include <string>
#include <fstream>
#include "xml_parser.h"

using namespace std;
using namespace pqxx;


class database {
private:
   connection *C = NULL; 

public:
    
    int init_database();

    int connect();

    // 0 for no error
    // 1 for account exist
    int handle_new_account(const create req);

    // 0 for no error
    // 1 for account not exist
    int handle_new_position(const create req);

    transct handle_sell(const transct req);

    transct handle_buy(const transct req);

    transct handle_query(const transct req);

    transct handle_cancel(const transct req);

    void print_account();

    void print_symbols();

    void print_open();

    void print_exe();

};

#endif