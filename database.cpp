# include "database.h"

int database::connect(){
try{
        //Establish a connection to the database
        //Parameters: database name, user name, user password
#if DOCKER
    C = new connection("dbname=stock user=postgres password=passw0rd host=stock_db port=5432");
#else
    C = new connection("dbname=stock user=postgres password=passw0rd");    
#endif
        if (!C->is_open()) {
            return 1;
        } 
    } catch (const std::exception &e){
        cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
};

int database::init_database(){
    try{
        //Establish a connection to the database
        //Parameters: database name, user name, user password
#if DOCKER
    C = new connection("dbname=stock user=postgres password=passw0rd host=stock_db port=5432");
#else
    C = new connection("dbname=stock user=postgres password=passw0rd");    
#endif
        if (C->is_open()) {
            cout << "Initialized database successfully: " << C->dbname() << endl;
        } else {
            return 1;
        }
    } catch (const std::exception &e){
        cerr << e.what() << std::endl;
        return 1;
    }

    //PLAYER TEAM STATE COLOR
    std::vector<std::string> table_list;
    table_list.push_back("OPEN");
    table_list.push_back("EXECUTED");
    table_list.push_back("CANCLED");
    table_list.push_back("TRANSACTION");
    table_list.push_back("POSITION");
    table_list.push_back("ACCOUNT");
    
    // Drop existing tables
    for(size_t i = 0; i < table_list.size(); i++){
        pqxx::work transaction(*C);
        std::string drop_table_query = "DROP TABLE IF EXISTS " + table_list[i] + ";";
        transaction.exec(drop_table_query);
        transaction.commit();
    }
    
    pqxx::work transaction(*C);
    std::string create_acc = R"(
        CREATE TABLE ACCOUNT (
        ACC_ID INTEGER PRIMARY KEY,
        BALANCE REAL NOT NULL CHECK (BALANCE >= 0)
        );
    )";

    transaction.exec(create_acc);
    
    std::string create_pos = 
    "CREATE TABLE POSITION ("
        "SYMBOL     VARCHAR,"
        "ACC_ID     INTEGER REFERENCES ACCOUNT (ACC_ID),"
        "NUM        REAL CHECK (NUM >= 0),"
        "PRIMARY KEY (SYMBOL, ACC_ID)"
    ")";
  
    transaction.exec(create_pos);

    std::string create_transct = 
    "CREATE TABLE TRANSACTION ("
        "TRAN_ID    SERIAL      PRIMARY KEY,"
        "ACC_ID     INTEGER     NOT NULL REFERENCES ACCOUNT (ACC_ID)"
    ")";
    
    transaction.exec(create_transct);

    std::string create_exe = 
    "CREATE TABLE EXECUTED ("
        "ID         SERIAL      PRIMARY KEY,"
        "TRAN_ID    INTEGER     NOT NULL REFERENCES TRANSACTION (TRAN_ID),"
        "SHARES     REAL        NOT NULL,"
        "PRICE      REAL        NOT NULL CHECK (PRICE > 0),"
        "TIME       TIMESTAMP   NOT NULL,"
        "SYMBOL     VARCHAR     NOT NULL"
    ")";
    
    transaction.exec(create_exe);

    std::string create_closed = 
    "CREATE TABLE CANCLED ("
        "ID         SERIAL      PRIMARY KEY,"
        "TRAN_ID    INTEGER     NOT NULL REFERENCES TRANSACTION (TRAN_ID),"
        "SHARES     REAL        NOT NULL,"
        "TIME       TIMESTAMP   NOT NULL,"
        "SYMBOL     VARCHAR     NOT NULL"
    ")";
    
    transaction.exec(create_closed);

    std::string create_open = 
    "CREATE TABLE OPEN ("
        "ID         SERIAL      PRIMARY KEY,"
        "TRAN_ID    INTEGER     NOT NULL REFERENCES TRANSACTION (TRAN_ID),"
        "SHARES     INTEGER     NOT NULL,"
        "TIME       TIMESTAMP   NOT NULL,"
        "PRICE      REAL        NOT NULL,"
        "SYMBOL     VARCHAR     NOT NULL"
    ")";
    
    transaction.exec(create_open);

    transaction.commit();
    C->disconnect();
    return 0;
}

// delete open order from open table, add two executed order
void deal(
    string open_o_id, string tran_id_1, string tran_id_2,
    string amount, string price, string symbol,
    pqxx::work* txn
)
{
    // delete dealed order from open
    std::string sql_lock = "SELECT * FROM open WHERE id = $1 FOR UPDATE";
    txn->exec_params(sql_lock, open_o_id);
    
    std::string sql_del = "DELETE FROM open WHERE id = $1";
    txn->exec_params(sql_del, open_o_id);
    
    // add two record in executed row
    std::string sql_add = 
    "INSERT INTO executed (tran_id, shares, price, time, symbol) " 
    "VALUES ($1, $2, $3, CURRENT_TIMESTAMP, $4)";
    txn->exec_params(sql_add, tran_id_1, amount, price, symbol);
    txn->exec_params(sql_add, tran_id_2, amount, price, symbol);
}

int database::handle_new_account(const create req)
{
    work W(*C);
    // create account
    
    try
    {
        subtransaction S(W);
        std::string sql = 
        "INSERT INTO ACCOUNT (ACC_ID, BALANCE) VALUES ("
        + std::to_string(req.acc_id) + "," 
        + std::to_string(req.balance) +
        ")";
        
        S.exec0(sql);
        S.commit();
    }
    catch (const std::exception &e)
    {
        return 1;
    }
    W.commit();
    return 0;
}

int database::handle_new_position(const create req)
{
    // Check if account exist
    pqxx::nontransaction txn(*C);
    
    std::string sql = 
    "SELECT * FROM account" 
    " WHERE acc_id = " + to_string(req.acc_id);

    pqxx::result res = txn.exec(sql);

    if (res.empty()) {
        std::cout << "Account not found" << std::endl;
        return 1;
    }
    txn.commit();

    pqxx::work W(*C);
    // try insert if exist update
    try
    {
        subtransaction S(W);
        std::string sql = 
        "INSERT INTO POSITION (SYMBOL, ACC_ID, NUM) VALUES ('" 
        + req.sym + "'" + "," 
        + std::to_string(req.acc_id) + ","
        + std::to_string(req.amount) + 
        ")";
        
        S.exec0(sql);
        S.commit();
    }
    catch (const std::exception &e)
    {
        subtransaction S(W);
        std::string query = 
        "UPDATE position SET num = num + " + std::to_string(req.amount) + 
        " WHERE acc_id = " + std::to_string(req.acc_id) +
        " AND symbol = '" + req.sym + "'";
        
        S.exec0(query);
        S.commit();
    }
    W.commit();
    return 0;
}

// in open table, shares > 0 means buy, shares < 0 means sell.
transct database::handle_sell(transct req){
    pqxx::work txn(*C);
    std::string acc_id = std::to_string(req.acc_id);
    std::string limit = std::to_string(req.limit);
    std::string amt = std::to_string( - req.amount);

    // verify position number and lock
    std::string sql_verify = 
    "SELECT NUM FROM position "
    "WHERE acc_id = " + acc_id + " AND symbol = " + txn.quote(req.sym) +  " FOR UPDATE;"; 

    // Execute the SQL statement and fetch the result
    pqxx::result r_verify = txn.exec(sql_verify);
    if (r_verify.empty()) {
        req.error_msg = "You don't hold this stock";
        txn.commit();
        return req;
    }
    if (r_verify[0]["num"].as<double>() < - req.amount){
        req.error_msg = "You don't have enough this stock";
        txn.commit();
        return req;
    }
    
    // reduce amount from seller's account
    std::string sql_update_num = 
    "UPDATE position SET NUM = NUM - " + amt +
    " WHERE acc_id = " + acc_id + " AND symbol = " + txn.quote(req.sym)+ ";";
    txn.exec0(sql_update_num);
    
    // create a transaction record
    pqxx::result res = txn.exec_params(
        "INSERT INTO TRANSACTION (ACC_ID) VALUES ($1) RETURNING TRAN_ID", acc_id);
    std::string tran_id = res[0][0].as<std::string>();
    
    // find all qualified opened buying orders
    std::string find_buyer = 
    "SELECT * FROM open WHERE "
    "shares > 0 AND price >= $1 AND symbol = $2 ORDER BY price DESC, time ASC FOR UPDATE;";
    pqxx::result all_buyers = txn.exec_params(find_buyer, limit, req.sym);

    double amt_remain = - req.amount;
    for (auto buyer : all_buyers)
    {
        int buyer_amt = buyer["shares"].as<int>();
        
        // add money to seller's account
        double fund_num = buyer_amt > amt_remain ? amt_remain : buyer_amt;
        std::string sql_update_bal = 
        "UPDATE account SET BALANCE = BALANCE + " + 
        to_string(fund_num * (buyer["price"].as<double>())) +
        " WHERE acc_id = " + acc_id + ";";
        txn.exec0(sql_update_bal);

        if (buyer_amt == amt_remain)
        {   
            deal(
                buyer["id"].as<string>(), tran_id, 
                buyer["tran_id"].as<string>(),
                to_string(buyer_amt), 
                buyer["price"].as<string>(), 
                req.sym, &txn
            );
            
            amt_remain = 0;
            break;
        }
        
        if(buyer_amt > amt_remain){

            deal(
                buyer["id"].as<string>(), tran_id, 
                buyer["tran_id"].as<string>(),
                to_string(buyer_amt), 
                buyer["price"].as<string>(), 
                req.sym, &txn
            );
            // put remaining amount in open table
            pqxx::result res = txn.exec_params(
                "INSERT INTO OPEN (TRAN_ID, SHARES, TIME, PRICE, SYMBOL)" 
                "VALUES ($1, $2, $3, $4, $5) RETURNING TRAN_ID"
                , buyer["tran_id"].as<string>(),
                buyer_amt - amt_remain,
                buyer["time"].as<string>(),
                buyer["price"].as<string>(),
                buyer["symbol"].as<string>()
            );
            
            amt_remain = 0;
            break;
        }
        
        if (buyer_amt < amt_remain)
        {
            deal(
                buyer["id"].as<string>(), tran_id, 
                buyer["tran_id"].as<string>(),
                to_string(buyer_amt), 
                buyer["price"].as<string>(), 
                req.sym, &txn
            );
            amt_remain -= buyer_amt;
            continue;
        } 
    }
    
    if(amt_remain > 0){
        // add in open
        // put remaining amount in open table
        pqxx::result res = txn.exec_params(
            "INSERT INTO OPEN (TRAN_ID, SHARES, TIME, PRICE, SYMBOL)" 
            "VALUES ($1, $2, CURRENT_TIMESTAMP, $3, $4) RETURNING TRAN_ID"
            , tran_id, - amt_remain, limit, req.sym
        );
    }
    txn.commit();
    req.transct_id = stoi(tran_id);
    return req;
}

transct database::handle_buy(transct req){
    pqxx::work txn(*C);
    std::string acc_id = std::to_string(req.acc_id);
    std::string limit = std::to_string(req.limit);
    std::string amt = std::to_string(req.amount);

    // verify position number and lock
    std::string sql_verify = 
    "SELECT BALANCE FROM account "
    "WHERE acc_id = " + acc_id + " FOR UPDATE;"; 

    // Execute the SQL statement and fetch the result
    pqxx::result r_verify = txn.exec(sql_verify);
    if (r_verify[0]["balance"].as<double>() < req.amount * req.limit){
        req.error_msg = "You don't have enough money";
        txn.commit();
        return req;
    }
    
    // reduce money from buyer's account
    std::string sql_update_bal = 
    "UPDATE account SET BALANCE = BALANCE - " + 
    to_string(req.amount * req.limit) +
    " WHERE acc_id = " + acc_id + ";";
    txn.exec0(sql_update_bal);
    
    // create a transaction record
    pqxx::result res = txn.exec_params(
        "INSERT INTO TRANSACTION (ACC_ID) VALUES ($1) RETURNING TRAN_ID", acc_id);
    std::string tran_id = res[0][0].as<std::string>();
    
    // find all qualified opened buying orders
    std::string find_buyer = 
    "SELECT * FROM open WHERE "
    "shares < 0 AND price <= $1 AND symbol = $2 ORDER BY price ASC, time ASC FOR UPDATE";
    pqxx::result all_sellers = txn.exec_params(find_buyer, limit, req.sym);

    double amt_remain = req.amount;
    for (auto seller : all_sellers)
    {
        double seller_amt = seller["shares"].as<double>();

        // buyer's refund
        double refund_num = seller_amt > amt_remain ? amt_remain : seller_amt;
        std::string sql_update_refund = 
        "UPDATE account SET BALANCE = BALANCE + " + 
        to_string(refund_num * (req.limit - seller["price"].as<double>())) +
        " WHERE acc_id = " + acc_id + ";";
        txn.exec0(sql_update_refund);
        
        if (seller_amt == amt_remain)
        {   
            deal(
                seller["id"].as<string>(), tran_id, 
                seller["tran_id"].as<string>(),
                to_string(seller_amt), 
                seller["price"].as<string>(), 
                req.sym, &txn
            );

            amt_remain = 0;
            break;
        }
        
        if(seller_amt > amt_remain){

            deal(
                seller["id"].as<string>(), tran_id, 
                seller["tran_id"].as<string>(),
                to_string(seller_amt), 
                seller["price"].as<string>(), 
                req.sym, &txn
            );

            // put remaining amount in open table
            pqxx::result res = txn.exec_params(
                "INSERT INTO OPEN (TRAN_ID, SHARES, TIME, PRICE, SYMBOL)" 
                "VALUES ($1, $2, $3, $4, $5) RETURNING TRAN_ID"
                , seller["tran_id"].as<string>(),
                seller_amt - amt_remain,
                seller["time"].as<string>(),
                seller["price"].as<string>(),
                seller["symbol"].as<string>()
            );
            
            amt_remain = 0;
            break;
        }
        
        if (seller_amt < amt_remain)
        {
            deal(
                seller["id"].as<string>(), tran_id, 
                seller["tran_id"].as<string>(),
                to_string(seller_amt), 
                seller["price"].as<string>(), 
                req.sym, &txn
            );
            amt_remain -= seller_amt;
            continue;
        } 
    }
    
    if(amt_remain > 0){
        // add in open
        // put remaining amount in open table
        pqxx::result res = txn.exec_params(
            "INSERT INTO OPEN (TRAN_ID, SHARES, TIME, PRICE, SYMBOL)" 
            "VALUES ($1, $2, CURRENT_TIMESTAMP, $3, $4) RETURNING TRAN_ID"
            , tran_id, amt_remain, limit, req.sym
        );
    }
    
    txn.commit();
    req.transct_id = stoi(tran_id);
    return req;
}

transct database::handle_query(const transct req){

}

transct database::handle_cancel(const transct req){

}


void database::print_account(){
    
    pqxx::work txn(*C);
    pqxx::result res = txn.exec("SELECT * FROM account");
    txn.commit();
    cout << "Account Table: " << endl;
    cout << "ACC_ID  " << "BALANCE"<<endl;
    for (auto row : res) {
        std::cout << row["acc_id"].as<std::string>() 
        << " " << row["balance"].as<int>() << std::endl;
    }
}

void database::print_symbols(){
    
    pqxx::work txn(*C);
    pqxx::result res = txn.exec("SELECT * FROM position");
    txn.commit();
    cout << "POSITION Table: " << endl;
    cout << "SYMBOL  " << "ACC_ID " << "NUM " << endl;
    for (auto row : res) {
        std::cout << row["SYMBOL"].as<std::string>() 
        << " " << row["ACC_ID"].as<int>() 
        << " " << row["NUM"].as<int>() << std::endl;
    }
}

void database::print_open(){
    
    pqxx::work txn(*C);
    pqxx::result res = txn.exec("SELECT * FROM open");
    txn.commit();
    cout << "OPEN Table: " << endl;
    cout << "ID  " << "TRAN_ID " << "SHARES " << "TIME " << "PRICE " << "SYMBOL " << endl;
    for (auto row : res) {
        std::cout << row["ID"].as<std::string>() 
        << " " << row["TRAN_ID"].as<string>() 
        << " " << row["SHARES"].as<string>()
        << " " << row["TIME"].as<string>()
        << " " << row["PRICE"].as<string>()
        << " " << row["SYMBOL"].as<string>() << std::endl;
    }
}


void database::print_exe(){
    
    pqxx::work txn(*C);
    pqxx::result res = txn.exec("SELECT * FROM EXECUTED");
    txn.commit();
    cout << "EXECUTED Table: " << endl;
    cout << "ID  " << "TRAN_ID " << "SHARES " << "TIME " << "PRICE " << "SYMBOL " << endl;
    for (auto row : res) {
        std::cout << row["ID"].as<std::string>() 
        << " " << row["TRAN_ID"].as<string>() 
        << " " << row["SHARES"].as<string>()
        << " " << row["TIME"].as<string>()
        << " " << row["PRICE"].as<string>()
        << " " << row["SYMBOL"].as<string>() << std::endl;
    }
}
