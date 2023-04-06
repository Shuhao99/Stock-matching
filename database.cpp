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

void database::disconnect(){
    C->disconnect();
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
        BALANCE NUMERIC NOT NULL CHECK (BALANCE >= 0)
        );
    )";

    transaction.exec(create_acc);
    
    std::string create_pos = 
    "CREATE TABLE POSITION ("
        "SYMBOL     VARCHAR,"
        "ACC_ID     INTEGER REFERENCES ACCOUNT (ACC_ID),"
        "NUM        NUMERIC CHECK (NUM >= 0),"
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
        "SHARES     NUMERIC        NOT NULL,"
        "PRICE      NUMERIC        NOT NULL CHECK (PRICE > 0),"
        "TIME       TIMESTAMP   NOT NULL,"
        "SYMBOL     VARCHAR     NOT NULL"
    ")";
    
    transaction.exec(create_exe);

    std::string create_closed = 
    "CREATE TABLE CANCLED ("
        "ID         SERIAL      PRIMARY KEY,"
        "TRAN_ID    INTEGER     NOT NULL REFERENCES TRANSACTION (TRAN_ID),"
        "SHARES     NUMERIC        NOT NULL,"
        "TIME       TIMESTAMP   NOT NULL,"
        "SYMBOL     VARCHAR     NOT NULL"
    ")";
    
    transaction.exec(create_closed);

    std::string create_open = 
    "CREATE TABLE OPEN ("
        "ID         SERIAL      PRIMARY KEY,"
        "TRAN_ID    INTEGER     NOT NULL REFERENCES TRANSACTION (TRAN_ID),"
        "SHARES     NUMERIC     NOT NULL,"
        "TIME       TIMESTAMP   NOT NULL,"
        "PRICE      NUMERIC        NOT NULL,"
        "SYMBOL     VARCHAR     NOT NULL"
    ")";
    
    transaction.exec(create_open);

    transaction.commit();
    C->disconnect();
    return 0;
}

// delete open order from open table, add two executed order
void database::deal(
    string open_o_id, string tran_id_buy, string tran_id_sell,
    string amount, string price, string symbol,
    pqxx::work* txn
)
{
    // delete dealed order from open
    std::string sql_lock = "SELECT * FROM open WHERE id = $1 FOR UPDATE";
    txn->exec_params(sql_lock, open_o_id);
    
    std::string sql_del = "DELETE FROM open WHERE id = $1";
    txn->exec_params(sql_del, open_o_id);
    
    // get buyer's id
    std::string get_b_id = 
    "SELECT ACC_ID FROM TRANSACTION WHERE TRAN_ID = $1 FOR UPDATE";
    pqxx::result res_b_id = txn->exec_params(get_b_id, tran_id_buy);
    string b_id = res_b_id[0]["ACC_ID"].as<string>();

    // get seller's id
    std::string get_s_id = 
    "SELECT ACC_ID FROM TRANSACTION WHERE TRAN_ID = $1 FOR UPDATE";
    pqxx::result res_s_id = txn->exec_params(get_s_id, tran_id_sell);
    string s_id = res_s_id[0]["ACC_ID"].as<string>();
    
    // seller add money
    std::string lock1 = 
    "SELECT * FROM account WHERE acc_id = $1 FOR UPDATE";
    txn->exec_params(lock1, s_id);
    
    std::string sql_update_refund = 
    "UPDATE account SET BALANCE = BALANCE + " + 
    to_string(stod(amount) * stod(price)) +
    " WHERE acc_id = " + s_id + ";";
    txn->exec0(sql_update_refund);
    
    // buyer add position
    std::string lock2 = 
    "SELECT * FROM POSITION "
    "WHERE acc_id = $1 AND SYMBOL = $2 "
    "FOR UPDATE";
    txn->exec_params(lock2, b_id, symbol);
    
    std::string sql_update_pos = 
    "UPDATE POSITION SET NUM = NUM + " + 
    amount +
    " WHERE acc_id = " + b_id + 
    " AND symbol = " + txn->quote(symbol) + ";";
    txn->exec0(sql_update_pos);

    // add two record in executed row
    // string :: sql_lock_ = 
    // "SELECT * INTO executed (tran_id, shares, price, time, symbol) " 
    // "VALUES ($1, $2, $3, CURRENT_TIMESTAMP, $4)";

    std::string sql_add = 
    "INSERT INTO executed (tran_id, shares, price, time, symbol) " 
    "VALUES ($1, $2, $3, CURRENT_TIMESTAMP, $4)";
    
    txn->exec_params(sql_add, tran_id_buy, amount, price, symbol);
    txn->exec_params(sql_add, tran_id_sell, "-" + amount, price, symbol);
}

int database::verify_acc_id(string acc_id){
    // Check if account exist
    pqxx::work txn(*C);
    
    std::string sql = 
    "SELECT * FROM account" 
    " WHERE acc_id = " + to_string(acc_id) + " FOR UPDATE";

    pqxx::result res = txn.exec(sql);

    if (res.empty()) {
        std::cout << "Account not found 2" << std::endl;
        txn.commit();
        return 0;
    }
    
    txn.commit();
    return 1;
}

int database::handle_new_account(const create req)
{
    pqxx::work W(*C);
    // create account
    string sql_lock =  
    "LOCK TABLE ACCOUNT IN ACCESS EXCLUSIVE MODE;";

    W.exec(sql_lock);
    
    try
    {
        // subtransaction S(W);
        std::string sql = 
        "INSERT INTO ACCOUNT (ACC_ID, BALANCE) VALUES ("
        + std::to_string(req.acc_id) + "," 
        + req.balance +
        ")";
        
        W.exec0(sql);
        W.commit();
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
    pqxx::work txn(*C);
    
    std::string sql = 
    "SELECT * FROM account" 
    " WHERE acc_id = " + to_string(req.acc_id) + " FOR UPDATE";

    
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
        + req.amount + 
        ")";
        
        S.exec0(sql);
        S.commit();
    }
    catch (const std::exception &e)
    {
        subtransaction S(W);
        std::string query = 
        "UPDATE position SET num = num + " + req.amount + 
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
    if(!verify_acc_id(to_string(req.acc_id))){
        // send response back
        req.error_msg = "Invalid account ID.";
        return req;
    }

    pqxx::work txn(*C);
    std::string acc_id = std::to_string(req.acc_id);
    std::string limit = req.limit;
    std::string amt = std::to_string( - stod(req.amount));
    
    // verify position number and lock
    std::string sql_verify = 
    "SELECT NUM FROM position "
    "WHERE acc_id = " + acc_id + " AND symbol = " + txn.quote(req.sym); 

    // Execute the SQL statement and fetch the result
    pqxx::result r_verify = txn.exec(sql_verify);
    if (r_verify.empty()) {
        req.error_msg = "You don't hold this stock";
        txn.commit();
        return req;
    }
    if (stod(r_verify[0]["num"].as<string>()) < - stod(req.amount)){
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

    double amt_remain = - stod(req.amount);
    for (auto buyer : all_buyers)
    {
        double buyer_amt = buyer["shares"].as<double>();
        if (amt_remain == 0)
        {
            break;
        }
        
        // add money to seller's account
        // double fund_num = buyer_amt > amt_remain ? amt_remain : buyer_amt;
        // std::string sql_update_bal = 
        // "UPDATE account SET BALANCE = BALANCE + " + 
        // to_string(fund_num * stod((buyer["price"].as<string>()))) +
        // " WHERE acc_id = " + acc_id + ";";
        // txn.exec0(sql_update_bal);

        if (buyer_amt == amt_remain)
        {   
            deal(
                buyer["id"].as<string>(), buyer["tran_id"].as<string>(), 
                tran_id,
                to_string(buyer_amt), 
                buyer["price"].as<string>(), 
                req.sym, &txn
            );
            
            amt_remain = 0;
            break;
        }
        
        if(buyer_amt > amt_remain){

            deal(
                buyer["id"].as<string>(), buyer["tran_id"].as<string>(), 
                tran_id,
                to_string(amt_remain), 
                buyer["price"].as<string>(), 
                req.sym, &txn
            );
            // put remaining amount in open table
            pqxx::result res = txn.exec_params(
                "INSERT INTO OPEN (TRAN_ID, SHARES, TIME, PRICE, SYMBOL)" 
                "VALUES ($1, $2, $3, $4, $5) RETURNING TRAN_ID"
                , buyer["tran_id"].as<string>(),
                to_string(buyer_amt - amt_remain),
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
                buyer["id"].as<string>(), buyer["tran_id"].as<string>(), 
                tran_id,
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
            , tran_id, "-" + to_string(amt_remain), limit, req.sym
        );
    }
    txn.commit();
    req.transct_id = stoi(tran_id);
    return req;
}

transct database::handle_buy(transct req){
    if(!verify_acc_id(to_string(req.acc_id))){
        // send response back
        req.error_msg = "Invalid account ID.";
        return req;
    }
    // create transaction for put buy order
    pqxx::work txn_red(*C);
    std::string acc_id = std::to_string(req.acc_id);
    std::string limit = req.limit;
    std::string amt = req.amount;

    // verify position number and lock
    std::string sql_verify = 
    "SELECT BALANCE FROM account "
    "WHERE acc_id = " + acc_id + " FOR UPDATE;"; 

    // Execute the SQL statement and fetch the result
    pqxx::result r_verify = txn_red.exec(sql_verify);
    if (stod(r_verify[0]["balance"].as<string>()) < stod(req.amount) * stod(req.limit)){
        req.error_msg = "You don't have enough money";
        txn_red.commit();
        return req;
    }
    
    // reduce money from buyer's account
    std::string sql_update_bal = 
    "UPDATE account SET BALANCE = BALANCE - (" + 
    req.amount + "*" + req.limit + ")" +
    " WHERE acc_id = " + acc_id + ";";
    txn_red.exec0(sql_update_bal);
    
    // create a transaction record
    pqxx::result res = txn_red.exec_params(
        "INSERT INTO TRANSACTION (ACC_ID) VALUES ($1) RETURNING TRAN_ID", acc_id);
    std::string tran_id = res[0][0].as<std::string>();
    txn_red.commit();

    // create transaction to handle match
    pqxx::work txn(*C);

    // find all qualified opened buying orders
    std::string find_buyer = 
    "SELECT * FROM open WHERE "
    "shares < 0 AND price <= $1 AND symbol = $2 ORDER BY price ASC, time ASC";
    pqxx::result all_sellers = txn.exec_params(find_buyer, limit, req.sym);

    double amt_remain = stod(req.amount);
    for (auto seller : all_sellers)
    {
        if (amt_remain == 0)
        {
            break;
        }
        
        double seller_amt = - stod(seller["shares"].as<string>());

        // buyer's refund number
        double refund_num = seller_amt > amt_remain ? amt_remain : seller_amt;

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
        }
        
        else if(seller_amt > amt_remain){

            deal(
                seller["id"].as<string>(), tran_id, 
                seller["tran_id"].as<string>(),
                to_string(amt_remain), 
                seller["price"].as<string>(), 
                req.sym, &txn
            );

            // put remaining amount in open table
            pqxx::result res = txn.exec_params(
                "INSERT INTO OPEN (TRAN_ID, SHARES, TIME, PRICE, SYMBOL)" 
                "VALUES ($1, $2, $3, $4, $5) RETURNING TRAN_ID"
                , seller["tran_id"].as<string>(),
                "-" + to_string(seller_amt - amt_remain),
                seller["time"].as<string>(),
                seller["price"].as<string>(),
                seller["symbol"].as<string>()
            );
            
            amt_remain = 0;
        }
        
        else if (seller_amt < amt_remain)
        {
            deal(
                seller["id"].as<string>(), tran_id, 
                seller["tran_id"].as<string>(),
                to_string(seller_amt), 
                seller["price"].as<string>(), 
                req.sym, &txn
            );
            amt_remain -= seller_amt;
        } 

        // buyer's refund
        std::string sql_update_refund = 
        "UPDATE account SET BALANCE = BALANCE + " + 
        to_string(refund_num * (stod(req.limit) - stod(seller["price"].as<string>()))) +
        " WHERE acc_id = " + acc_id + ";";
        txn.exec0(sql_update_refund);
    }
    
    if(amt_remain > 0){
        // add in open
        // put remaining amount in open table
        pqxx::result res = txn.exec_params(
            "INSERT INTO OPEN (TRAN_ID, SHARES, TIME, PRICE, SYMBOL)" 
            "VALUES ($1, $2, CURRENT_TIMESTAMP, $3, $4) RETURNING TRAN_ID"
            , tran_id, to_string(amt_remain), limit, req.sym
        );
    }
    
    txn.commit();
    req.transct_id = stoi(tran_id);
    return req;
}

transct database::handle_query(transct req){
    if(!verify_acc_id(to_string(req.acc_id))){
        // send response back
        req.error_msg = "Invalid account ID.";
        return req;
    }
    // Check if require is valid
    int status = verify_tranxt(to_string(req.acc_id), to_string(req.transct_id));

    if (status == 1){
        req.error_msg = "This transaction ID is invalid.";
        return req;
    }
    if (status == 2){
        req.error_msg = "This transaction is not valid.";
        return req;
    }

    pqxx::work txn(*C);
    std::string acc_id = std::to_string(req.acc_id);
    std::string tran_id = std::to_string(req.transct_id);
    
    // get opened
    std::string find_open = 
    "SELECT SHARES FROM open WHERE "
    "TRAN_ID = $1 FOR UPDATE;";
    pqxx::result opens = txn.exec_params(find_open, tran_id);
    if(!opens.empty()){
        req.qr.open_shares = opens[0]["SHARES"].as<string>();
    }

    // get canceled
    std::string find_cles = 
    "SELECT SHARES, TIME FROM CANCLED WHERE "
    "TRAN_ID = $1 FOR UPDATE;";
    pqxx::result cles = txn.exec_params(find_cles, tran_id);
    if(!cles.empty()){
        req.qr.cancled_shares = cles[0]["SHARES"].as<string>();
        req.qr.cancled_time = trans_time(cles[0]["TIME"].as<string>());
    }

    // get executed 
    req.qr.exes = get_exes(&txn, tran_id);
    
    txn.commit();
    return req;
}

transct database::handle_cancel(transct req){
    if(!verify_acc_id(to_string(req.acc_id))){
        // send response back
        req.error_msg = "Invalid account ID.";
        return req;
    }
    // Check if require is valid
    int status = verify_tranxt(to_string(req.acc_id), to_string(req.transct_id));

    if (status == 1){
        req.error_msg = "This transaction ID is invalid.";
        return req;
    }
    if (status == 2){
        req.error_msg = "This transaction is not yours.";
        return req;
    }

    pqxx::work txn(*C);
    std::string acc_id = std::to_string(req.acc_id);
    std::string tran_id = std::to_string(req.transct_id);

    // delete order from open table
    std::string sql_lock = "SELECT * FROM open WHERE TRAN_ID = $1 FOR UPDATE";
    pqxx::result res = txn.exec_params(sql_lock, tran_id);

    if (res.empty())
    {
        req.error_msg = "This order already canceled or executed.";
        return req;
    }
    

    string shares = res[0]["shares"].as<string>();
    double shares_d = stod(res[0]["shares"].as<string>());
    double price_d = stod(res[0]["price"].as<string>());
    string symbol = res[0]["symbol"].as<string>();
    
    std::string sql_del = "DELETE FROM open WHERE TRAN_ID = $1";
    txn.exec_params(sql_del, tran_id);

    // add this order to cancled table
    std::string sql_add = 
    "INSERT INTO CANCLED (tran_id, shares, time, symbol) " 
    "VALUES ($1, $2, CURRENT_TIMESTAMP, $3)";
    txn.exec_params(sql_add, tran_id, shares, symbol);
    
    std::time_t now = std::time(nullptr);
    std::string time_str = std::to_string(now);

    req.cr.cancled_time = time_str;
    req.cr.cancled_shares = shares;

    // get executed orders
    req.cr.exes = get_exes(&txn, tran_id);

    // refund
    
    // buyer's refund
    if(shares_d > 0){
        std::string lock1 = 
        "SELECT * FROM account WHERE acc_id = $1 FOR UPDATE";
        txn.exec_params(lock1, acc_id);
        
        std::string sql_update_refund = 
        "UPDATE account SET BALANCE = BALANCE + " + 
        to_string(shares_d * price_d) +
        " WHERE acc_id = " + acc_id + ";";
        txn.exec0(sql_update_refund);
    }
    // seller's refund
    if(shares_d < 0){
        std::string lock2 = 
        "SELECT * FROM POSITION "
        "WHERE acc_id = $1 AND SYMBOL = $2 "
        "FOR UPDATE";
        txn.exec_params(lock2, acc_id, symbol);
        
        std::string sql_update_refund = 
        "UPDATE POSITION SET NUM = NUM + " + 
        to_string(- shares_d) +
        " WHERE acc_id = " + acc_id + 
        " AND symbol = " + txn.quote(symbol) + ";";
        txn.exec0(sql_update_refund);
    }
    
    txn.commit();
    return req;
}

vector<exed> database::get_exes(work* txn, string tran_id){
    // delete order from executed table
    std::string sql = 
        "SELECT * FROM EXECUTED WHERE TRAN_ID = $1 FOR UPDATE";
    pqxx::result res = txn->exec_params(sql, tran_id);
    vector<exed> exes;

    for(auto row : res){
        exed temp;
        temp.exe_price = row["price"].as<string>();
        temp.exe_shares = row["shares"].as<string>();
        string timestamp = row["time"].as<string>();
        temp.exe_time = trans_time(timestamp);
        exes.push_back(temp);
    }
    return exes;
}

int database::verify_tranxt(string acc_id, string tran_id){
    // verify transaction exist
    pqxx::work txn(*C);
    
    std::string sql = 
    "SELECT * FROM TRANSACTION" 
    " WHERE TRAN_ID = " + to_string(tran_id) + " FOR UPDATE";

    pqxx::result res = txn.exec(sql);

    if (res.empty()) {
        // can not find this trxt record
        txn.commit();
        return 1;
    }

    // verify transaction belong to account
    std::string sql2 = 
    "SELECT ACC_ID FROM TRANSACTION" 
    " WHERE TRAN_ID = " + to_string(tran_id) + " FOR UPDATE";
    pqxx::result res2 = txn.exec(sql2);

    if (res2[0]["acc_id"].as<string>() != acc_id) {
        // This trxt belong to other user
        txn.commit();
        return 2;
    }
    
    txn.commit();
    return 0;
}

string database::trans_time(const string timestamp){
    std::tm timeinfo = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    auto microseconds = std::stoul(timestamp.substr(20, 6));
    auto seconds = std::mktime(&timeinfo) + microseconds / 1000000;
    return to_string(seconds);
}

void database::print_account(){
    
    pqxx::work txn(*C);
    //pqxx::result res = txn.exec("SELECT * FROM account");
    pqxx::result res = txn.exec(
        "SELECT a.*, p.* FROM account a JOIN position p ON a.acc_id = p.acc_id"
    );
    txn.commit();
    cout << "----------------------------------------------------------------" << endl;
    cout << "Account Table: " << endl;
    cout << "----------------------------------------------------------------" << endl;
    cout << "ACC_ID  " << "  " << "BALANCE  " << "  " << "SYMBOL  " << "  " << "NUM  " << endl;
    for (auto row : res) {
        std::cout << row["acc_id"].as<std::string>() 
        << " " << row["balance"].as<string>() 
        << " " << row["symbol"].as<string>()
        << " " << row["num"].as<string>()
        << std::endl;
    }
}

void database::print_open(){
    
    pqxx::work txn(*C);
    pqxx::result res = txn.exec("SELECT * FROM open");
    txn.commit();
    cout << "----------------------------------------------------------------" << endl;
    cout << "OPEN Table: " << endl;
    cout << "----------------------------------------------------------------" << endl;
    cout << "ID  " << "  " << "TRAN_ID " << "  " << "SHARES " << "  " << "TIME " << "  " << "PRICE " << "  " << "SYMBOL " << endl;
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
    cout << "----------------------------------------------------------------" << endl;
    cout << "EXECUTED Table: " << endl;
    cout << "----------------------------------------------------------------" << endl;
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

void database::print_canceled(){
    
    pqxx::work txn(*C);
    pqxx::result res = txn.exec("SELECT * FROM CANCLED");
    txn.commit();
    cout << "----------------------------------------------------------------" << endl;
    cout << "CANCLED Table: " << endl;
    cout << "----------------------------------------------------------------" << endl;
    cout << "ID  " << "TRAN_ID  " << "SHARES  " << "TIME  " << "SYMBOL  " << endl;
    for (auto row : res) {
        std::cout << row["ID"].as<std::string>() 
        << " " << row["TRAN_ID"].as<string>() 
        << " " << row["SHARES"].as<string>()
        << " " << row["TIME"].as<string>()
        << " " << row["SYMBOL"].as<string>() << std::endl;
    }
}
