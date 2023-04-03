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
        BALANCE INTEGER NOT NULL CHECK (BALANCE >= 0)
        );
    )";

    transaction.exec(create_acc);
    
    std::string create_pos = 
    "CREATE TABLE POSITION ("
        "SYMBOL     VARCHAR,"
        "ACC_ID     INTEGER REFERENCES ACCOUNT (ACC_ID),"
        "NUM        INTEGER CHECK (NUM >= 0),"
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
        "SHARES     INTEGER     NOT NULL,"
        "PRICE      REAL        NOT NULL CHECK (PRICE > 0),"
        "TIME       BIGINT      NOT NULL"
    ")";
    
    transaction.exec(create_exe);

    std::string create_closed = 
    "CREATE TABLE CANCLED ("
        "ID         SERIAL      PRIMARY KEY,"
        "TRAN_ID    INTEGER     NOT NULL REFERENCES TRANSACTION (TRAN_ID),"
        "SHARES     INTEGER     NOT NULL,"
        "TIME       BIGINT      NOT NULL"
    ")";
    
    transaction.exec(create_closed);

    std::string create_open = 
    "CREATE TABLE OPEN ("
        "ID         SERIAL      PRIMARY KEY,"
        "TRAN_ID    INTEGER     NOT NULL REFERENCES TRANSACTION (TRAN_ID),"
        "SHARES     INTEGER     NOT NULL,"
        "TIME       BIGINT      NOT NULL"
    ")";
    
    transaction.exec(create_open);

    transaction.commit();
    C->disconnect();
    return 0;
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
