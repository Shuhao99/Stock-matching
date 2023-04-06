#ifndef DATABASE_H
#define DATABASE_H
#include <iostream>
#include <pqxx/pqxx>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>
#include "xml_parser.h"

using namespace std;
using namespace pqxx;


class database {
private:
   connection *C = NULL; 

public:
    
    int init_database();

    int connect();

    void disconnect();

    // 0 for no error
    // 1 for account exist
    int handle_new_account(const create req);

    // 0 for no error
    // 1 for account not exist
    int handle_new_position(const create req);

    transct handle_sell(const transct req);

    transct handle_buy(const transct req);

    transct handle_query(transct req);

    transct handle_cancel(transct req);

    // return 0 if not find, return 1 if find
    int verify_acc_id(string acc_id);

    // return 1 for no trxt finded
    // return 2 for tran_id belong to other user
    // return 0 for ok
    int verify_tranxt(string acc_id, string tran_id);
    
    void deal(
        string open_o_id, string tran_id_1, string tran_id_2,
        string amount, string price, string symbol,
        pqxx::work* txn
    );

    vector<exed> get_exes(work* txn, string tran_id);

    string trans_time(const string timestamp);

    void print_account();

    void print_canceled();

    void print_open();

    void print_exe();

};

#endif