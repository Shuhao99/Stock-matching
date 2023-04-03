#ifndef XML_PARSER_H
#define XML_PARSER_H
#include <vector>
#include <string>
#include <iostream>
#include "pugixml/pugixml.hpp"

using namespace pugi;

struct create
{
    std::string type; // account, position
    std::string sym;
    std::string err_msg = "";

    int amount = 0;
    int balance = 0;

    int acc_id = 0;

    void set_account(int acc_id_, int balance_) {
        acc_id = acc_id_;
        balance = balance_;
        type = "account";
    }

    void set_symbol(int acc_id_, int amt_, std::string sym_) {
        acc_id = acc_id_;
        amount = amt_;
        sym = sym_;
        type = "symbol";
    }
};

struct order
{
    std::string sym = NULL;
    int acc_id;
    int amount;
    int limit;
};

struct cancel
{
    int acc_id;
    int transct_id;
};



struct query
{
    int acc_id;
    int transct_id;
};

struct transct
{
    std::vector<order> orders;
    std::vector<cancel> cancels;
    std::vector<query> queries;
};


// acount int acc_id, int balance
// position string sys, int acc_id, int amount
// order string sys, int acc_id, int amount, int limit
// cancel string type, int trans_id
// query

const std::string get_root (const std::string xml_string);

const transct get_transaction (std::string xml);

const std::vector<create> get_create(std::string xml);

#endif
