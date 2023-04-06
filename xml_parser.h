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

    std::string amount = "";
    std::string balance = "";

    int acc_id = 0;

    void set_account(int acc_id_, std::string balance_) {
        acc_id = acc_id_;
        balance = balance_;
        type = "account";
    }

    void set_symbol(int acc_id_, std::string amt_, std::string sym_) {
        acc_id = acc_id_;
        amount = amt_;
        sym = sym_;
        type = "symbol";
    }
};

struct exed
{
    std::string exe_shares = "";
    std::string exe_price = "";
    std::string exe_time = "";
};


struct query_res
{
    std::string open_shares = "";
    std::string cancled_shares = "";
    std::string cancled_time = "";
    std::vector<exed> exes;
};

struct cancel_res
{
    std::string cancled_shares = "";
    std::string cancled_time = "";
    std::vector<exed> exes;
};

struct transct
{
    std::string type = "";
    std::string error_msg = "";
    std::string sym = "";
    
    int acc_id;
    int transct_id;

    std::string amount;
    std::string limit;

    query_res qr;
    cancel_res cr;

    void set_order(std::string sym_, int acc_id_, std::string amt_, std::string limit_) {
        acc_id = acc_id_;
        amount = amt_;
        sym = sym_;
        limit = limit_;
        type = "order";
    }

    void set_query(int acc_id_, int transct_id_) {
        acc_id = acc_id_;
        transct_id = transct_id_;
        type = "query";
    }

    void set_cancel(int acc_id_, int transct_id_) {
        acc_id = acc_id_;
        transct_id = transct_id_;
        type = "cancel";
    }
};

const std::string get_root (const std::string xml_string);

const std::vector<transct> get_transaction (std::string xml_str);

const std::vector<create> get_create(std::string xml);

#endif
