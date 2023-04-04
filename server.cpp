#include "server.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
std::ofstream log_file("trade.log");

void server::run(){
    database* db = new database();
    if(db->init_database()){
        cerr << "Can not connect to database." << std::endl;
    } 
    delete db;
    this->connection_lisn_fd = build_listener(lisn_port);
    if (this->connection_lisn_fd == -1)
    {
        pthread_mutex_lock(&mutex);
        log_file << "(no id): ERROR proxy server initialize failed." << std::endl;
        pthread_mutex_unlock(&mutex);
    }

    while (true)
    {
        std::string ip;
        int session_fd = create_session(connection_lisn_fd, &ip);
        if (session_fd < 0)
        {
            pthread_mutex_lock(&mutex);
            log_file << "(no id): ERROR proxy server connect failed." << std::endl;
            pthread_mutex_unlock(&mutex);
        }
        
        session temp = {session_fd, ip};
        this->session_queue.push_back(temp);
        pthread_t thread;
        pthread_create(&thread, NULL, handle, &(this->session_queue.back()));
    }

}

void * server::handle(void *curr_session_){
    //Receive xml from user
    cout << "Enter handle" << endl;
    session * curr_session = (session*)curr_session_;
    std::string xml_msg = get_xml(curr_session);
    
    if (xml_msg.empty() || xml_msg == "")
    {
       cerr << "Get Empty request from client." << endl;
       return NULL;
    }
    
    //parse xml, get root tag name
    const string root = get_root(xml_msg);
    string resp = "";
    
    // create a database object and connect
    database *db = new database();
    if(db->connect()){
        cerr << "Can not connect to database." << std::endl;
    };

    // create
    if(root == "create"){
        const std::vector<create> reqs = get_create(xml_msg);
        std::vector<create> responses;
        for (create req : reqs){
            if(req.type == "account"){
                handle_create(db, req, &responses);
            } 
            if(req.type == "symbol"){
                handle_symbol(db, req, &responses);
            }
        }
        // TODO: generate response
        resp = generate_create_resp(responses);
        db->print_account();
        db->print_symbols();
    }
    else if(root == "transactions"){
        resp = handle_tranxt(db, xml_msg);
        db->print_open();
        db->print_exe();
    }
    else{
        cerr << "format wrong" << endl;
    }

    // send response back
    send(curr_session->fd, resp.c_str(), resp.length(), 0);
    
    db->disconnect();
    delete db;
    close(curr_session->fd);
    return NULL;
}


string server::get_xml(
    const session* curr_session)
{
    //receive first part of message
    char buffer[BUFFER_SIZE];
    int rec_len = recv(curr_session->fd, buffer, sizeof(buffer), 0);
    if (rec_len <= 0)
    {
        return "";
    }
    //convert response messge to string
    std::string msg(buffer, rec_len);
    
    int num;

    std::istringstream iss(msg);
    iss >> num;
    
    // find first non-digit
    size_t non_digit_pos = msg.find_first_not_of("0123456789/n");

    std::string xml_msg = msg.substr(non_digit_pos);

    //get the whole msg package
    while ( rec_len > 0 && xml_msg.length() < num)
    {
        rec_len = recv(curr_session->fd, buffer, sizeof(buffer), 0);
        if (rec_len > 0){
            std::string temp(buffer, rec_len);
            xml_msg = xml_msg + temp;
        }
    }

    pthread_mutex_lock(&mutex);
    log_file << xml_msg << std::endl;
    pthread_mutex_unlock(&mutex);

    return xml_msg;
}

string server::handle_tranxt(database* const db, string xml_msg){
    const std::vector<transct> reqs = get_transaction(xml_msg);
    std::vector<transct> responses;
    string resp = "";
    
    // verify account if not generate_wrong_acc_id_resp
    if(!db->verify_acc_id(to_string(reqs[0].acc_id))){
        // send response back
        resp = generate_wrong_acc_id_resp(reqs);
        return resp;
    }

    for (transct req : reqs)
    {
        if(req.type == "order")
        {
            if(req.limit <= 0){
                req.error_msg = "The order's limit price is illegal, should >= 0";
                responses.push_back(req);
                continue;
            }
            if(req.amount == 0){
                req.error_msg = "The order's amount is not valid, should > 0";
                responses.push_back(req);
                continue;
            }
            
            // sell 
            if(req.amount < 0){
                transct res = db->handle_sell(req);
                if(res.error_msg == ""){
                    cout << res.transct_id << ": created sell" << endl;
                }
                else{
                    cout << ": error, " + res.error_msg << endl;
                }
                continue;
            }

            // buy
            if(req.amount > 0){
                transct res = db->handle_buy(req);
                if(res.error_msg == ""){
                    cout << res.transct_id << ": created buy" << endl;
                }
                else{
                    cout << ": error, " + res.error_msg << endl;
                }
                continue;
            }
        }
        
        if(req.type == "cancel")
        {
            cout << "Cancel:" << " " << req.acc_id 
            << " " << req.transct_id << endl;
        }
        
        if(req.type == "query")
        {
            cout << "Query:" << " " << req.acc_id 
            << " " << req.transct_id << endl;
        }
    }
    return resp;
}

void server::handle_create(database* const db, const create req, std::vector<create> *responses){
    // create account 
    // try insert catch error exist
    if(req.balance < 0){
        create res = req;
        res.err_msg = "Balance is not positive";
        cerr << res.err_msg << std::endl;
        responses->push_back(res);
        return;
    }

    if(db->handle_new_account(req)){
        // TODO: add sth in response
        create res = req;
        res.err_msg = "Account already exist";
        cerr << res.err_msg << std::endl;
        responses->push_back(res);
        return;
    }
    responses->push_back(req);
    return;
}

void server::handle_symbol(database* const db, const create req, std::vector<create> *responses){
    if(req.amount < 0){
        create res = req;
        res.err_msg = "Symbol ammount must be positive";
        cerr << res.err_msg << std::endl;
        responses->push_back(res);
        return;
    }
    if(db->handle_new_position(req)){
        create res = req;
        res.err_msg = "Account not found";
        cerr << res.err_msg << std::endl;
        responses->push_back(res);
        return;
    }
    responses->push_back(req);
    return;
}

int server::create_session(const int &listener_fd, std::string * ip){
    struct sockaddr_storage socket_addr;

    socklen_t socket_addr_len = sizeof(socket_addr);

    int client_connect_fd = accept(listener_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    
    struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
    
    *ip = inet_ntoa(addr->sin_addr);
    return client_connect_fd;
}


