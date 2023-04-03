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
    session * curr_session = (session*)curr_session_;
    std::string xml_msg = get_xml(curr_session);
    
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

    }
    else{
        cerr << "format wrong" << endl;
    }

    // send response back
    cout << resp << endl;
    send(curr_session->fd, resp.c_str(), resp.length() + 1, 0);
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
        return NULL;
    }
    //convert response messge to string
    std::string msg(buffer, rec_len);

    //get the whole msg package
    while ( rec_len > 0)
    {
        rec_len = recv(curr_session->fd, buffer, sizeof(buffer), 0);
        if (rec_len > 0){
            std::string temp(buffer, rec_len);
            msg = msg + temp;
        }
    }

    // find first non-digit
    size_t non_digit_pos = msg.find_first_not_of("0123456789");

    std::string xml_msg = msg.substr(non_digit_pos);

    pthread_mutex_lock(&mutex);
    log_file << xml_msg << std::endl;
    pthread_mutex_unlock(&mutex);

    return xml_msg;
}

void server::handle_create(database *db, const create req, std::vector<create> *responses){
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

void server::handle_symbol(database *db, const create req, std::vector<create> *responses){
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
