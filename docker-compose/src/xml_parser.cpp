#include "xml_parser.h"

const std::string get_root(const std::string xml_string){
    // parse XML string
    xml_document doc;
    doc.load_string(xml_string.c_str());

    // Get root node
    xml_node root = doc.document_element();
    if (root.empty()) {
        return "";
    }
    return std::string(root.name());
}

const std::vector<transct> get_transaction (std::string xml_str){
    
    std::vector<transct> temp;
    std::vector<transct> trxts;
   
    xml_document doc;
    doc.load_string(xml_str.c_str());
    xml_node root = doc.document_element();
    
    try
    {
        int curr_acc_id = std::stoi(std::string(root.attribute("id").value())); 

        for (xml_node node = root.first_child(); node; node = node.next_sibling()) {
            if (std::string(node.name()) == "order") 
            {
                const char* sym = node.attribute("sym").value();
                const char* amt = node.attribute("amount").value();
                const char* limit = node.attribute("limit").value();
                
                transct t;
                t.set_order(std::string(sym), curr_acc_id, std::string(amt), std::string(limit));
                temp.push_back(t);
            } 
            
            else if (std::string(node.name()) == "cancel") 
            {
                const char* trans_id = node.attribute("id").value();
                int trans_id_i = std::stoi(std::string(trans_id));
                
                transct t;
                t.set_cancel(curr_acc_id, trans_id_i);
                temp.push_back(t);
            }
            
            else if (std::string(node.name()) == "query") 
            {
                const char* trans_id = node.attribute("id").value();
                int trans_id_i = std::stoi(std::string(trans_id));
                
                transct t;
                t.set_query(curr_acc_id, trans_id_i);
                temp.push_back(t);
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return trxts;
    }

    trxts = temp;
    return trxts; 
}

const std::vector<create> get_create(std::string xml_str){
    std::vector<create> temp;
    std::vector<create> res;
    xml_document doc;
    doc.load_string(xml_str.c_str());
    xml_node root = doc.document_element();

    try
    {
        for (xml_node node = root.first_child(); node; node = node.next_sibling()) {
            if (std::string(node.name()) == "account") {
                const char* id = node.attribute("id").value();
                const char* balance = node.attribute("balance").value();
                int id_i = std::stoi(std::string(id));
                create c;
                c.set_account(id_i, std::string(balance));
                temp.push_back(c);
            } 
            else if (std::string(node.name()) == "symbol") {
                const std::string sym_ = std::string(node.attribute("sym").value());
                for (xml_node sym_node = node.first_child(); sym_node; sym_node = sym_node.next_sibling()){
                    
                    const char* id = sym_node.attribute("id").value();
                    const char* amt = sym_node.text().get();
                    int id_i = std::stoi(std::string(id));

                    create c;
                    c.set_symbol(id_i, std::string(amt), sym_);
                    temp.push_back(c);
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return res;
    }
    

    res = temp;
    return res;
}