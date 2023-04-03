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

const transct get_transaction (std::string xml){
    transct tran;
    return tran;
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
                int balance_i = std::stoi(std::string(balance));
                create c;
                c.set_account(id_i, balance_i);
                temp.push_back(c);
            } 
            else if (std::string(node.name()) == "symbol") {
                const std::string sym_ = std::string(node.attribute("sym").value());
                for (xml_node sym_node = node.first_child(); sym_node; sym_node = sym_node.next_sibling()){
                    const char* id = sym_node.attribute("id").value();
                    const char* amt = sym_node.text().get();
                    int id_i = std::stoi(std::string(id));
                    int amt_i = std::stoi(std::string(amt));
                    create c;
                    c.set_symbol(id_i, amt_i, sym_);
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