#include "server.h"

server::server(const char * port, int num_threads = std::thread::hardware_concurrency()) : 
lisn_port(port), 
connection_lisn_fd(-1),
stop_(false)
{
    // thread pool
    for (int i = 0; i < num_threads; ++i) {
        threads_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex_);
                    this->condition_.wait(lock, [this] { return this->stop_ || !this->tasks_.empty(); });
                    if (this->stop_ && this->tasks_.empty()) {
                        return;
                    }
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }
                task();
            }
        });
    }
}

server::~server() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (std::thread& thread : threads_) {
        thread.join();
    }
}

template<typename... Args>
void server::AddTask(void(*function)(Args...), Args... args) {
    auto task = std::bind(function, std::forward<Args>(args)...);
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        tasks_.emplace([task] {
            task();
        });
    }
    condition_.notify_one();
}

void server::run(){
    database* db = new database();
    if(db->init_database()){
        cerr << "Can not connect to database." << std::endl;
    } 
    delete db;
    this->connection_lisn_fd = build_listener(lisn_port);
    if (this->connection_lisn_fd == -1)
    {
        cerr << "ERROR proxy server initialize failed." << std::endl;
    }

    while (true)
    {
        std::string ip;
        int session_fd = create_session(connection_lisn_fd, &ip);
        if (session_fd < 0)
        {
            cerr << "ERROR proxy server initialize failed." << std::endl;
        }
       
        session temp = {session_fd, ip};
        this->AddTask(handle, temp);
    }

}

void server::handle(session curr_session){
    //Receive xml from user
    std::string xml_msg = get_xml(& curr_session);
    
    if (xml_msg.empty() || xml_msg == "")
    {
       cerr << "Get Empty request from client." << endl;
       return;
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
    }
    else if(root == "transactions"){
        resp = handle_tranxt(db, xml_msg);
    }
    else{
        cerr << "format wrong" << endl;
    }

    // send response back
    send(curr_session.fd, resp.c_str(), resp.length(), 0);
    
    // cout << endl;
    // cout<<"New trxt: " << endl;
    // cout << endl;

    // db->print_account();
    // db->print_open();
    // db->print_exe();
    // db->print_canceled();

    db->disconnect();
    delete db;
    close(curr_session.fd);
    return;
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

    if(num <= 0){
        return "";
    }
    
    // find first non-digit
    size_t non_digit_pos = msg.find_first_not_of("0123456789/n");

    std::string xml_msg = msg.substr(non_digit_pos);

    //get the whole msg package
    while ( rec_len > 0 && (int)xml_msg.length() < num)
    {
        rec_len = recv(curr_session->fd, buffer, sizeof(buffer), 0);
        if (rec_len > 0){
            std::string temp(buffer, rec_len);
            xml_msg = xml_msg + temp;
        }
    }

    return xml_msg;
}

string server::handle_tranxt(database* const db, string xml_msg){
    const std::vector<transct> reqs = get_transaction(xml_msg);
    std::vector<transct> responses;
    string resp = "";
    
    // verify account if not generate_wrong_acc_id_resp
    if (reqs.empty())
    {
        cout << "Get empty transaction" << endl;
        return resp;
    }

    for (transct req : reqs)
    {
        if(req.type == "order")
        {
            if(stod(req.limit) <= 0){
                req.error_msg = "The order's limit price is illegal, should >= 0";
                responses.push_back(req);
                continue;
            }
            if(stod(req.amount) == 0){
                req.error_msg = "The order's amount is not valid, should > 0";
                responses.push_back(req);
                continue;
            }
            
            // sell 
            if(stod(req.amount) < 0){
                transct res = db->handle_sell(req);
                responses.push_back(res);
                continue;
            }

            // buy
            if(stod(req.amount) > 0){
                transct res = db->handle_buy(req);
                responses.push_back(res);
                continue;
            }
        }
        
        if(req.type == "cancel")
        {
            transct resp = db->handle_cancel(req);
            responses.push_back(resp);
            continue;
        }
        
        if(req.type == "query")
        {
            transct resp = db->handle_query(req);
            responses.push_back(resp);
            continue;
        }
    }

    resp = generate_trxn_resp(responses);
    return resp;
}

void server::handle_create(database* const db, const create req, std::vector<create> *responses){
    // create account 
    // try insert catch error exist
    if(stod(req.balance) < 0){
        create res = req;
        res.err_msg = "Balance is not positive";
        responses->push_back(res);
        return;
    }

    if(db->handle_new_account(req)){
        create res = req;
        res.err_msg = "Account already exist";
        responses->push_back(res);
        return;
    }
    responses->push_back(req);
    return;
}

void server::handle_symbol(database* const db, const create req, std::vector<create> *responses){
    if(stod(req.amount) < 0){
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


