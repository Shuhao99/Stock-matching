#include "xml_generator.h"

std::string generate_create_resp(std::vector<create> responses)
{
    pugi::xml_document doc;

    // Add a root element to the document
    pugi::xml_node root = doc.append_child("results");
    for (create resp : responses)
    {
        if (resp.type == "account")
        {
            if (resp.err_msg == "")
            {
                pugi::xml_node node = root.append_child("created");
                node.append_attribute("id") = resp.acc_id;
            }
            else
            {
                pugi::xml_node node = root.append_child("error");
                node.append_attribute("id") = resp.acc_id;
                node.append_child(pugi::node_pcdata).set_value(resp.err_msg.c_str());
            }
        }
        if (resp.type == "symbol")
        {
            if (resp.err_msg == "")
            {
                pugi::xml_node node = root.append_child("created");
                node.append_attribute("sym") = resp.sym.c_str();
                node.append_attribute("id") = resp.acc_id;
            }
            else
            {
                pugi::xml_node node = root.append_child("error");
                node.append_attribute("sym") = resp.sym.c_str();
                node.append_attribute("id") = resp.acc_id;
                node.append_child(pugi::node_pcdata).set_value(resp.err_msg.c_str());
            }
        }
    }
    
    // Serialize the XML document to a string
    std::stringstream strs;
    doc.save(strs);

    // Print the generated XML text
    return strs.str();
}

// Generate response for transaction with invalid account id
std::string generate_wrong_acc_id_resp(
    std::vector<transct> responses
)
{
    pugi::xml_document doc;

    // Add a root element to the document
    pugi::xml_node root = doc.append_child("results");
    for (auto resp : responses)
    {
        if (resp.type == "order")
        {
            pugi::xml_node node = root.append_child("error");
            node.append_attribute("sym") = resp.sym.c_str();
            node.append_attribute("amount") = resp.amount.c_str();
            node.append_attribute("limit") = resp.limit.c_str();
            node.append_attribute("id") = resp.acc_id;
            node.append_child(pugi::node_pcdata).set_value("Account ID doesn't exist.");

        }
        if (resp.type == "cancel")
        {
            pugi::xml_node node = root.append_child("canceled");
            node.append_attribute("id") = resp.transct_id;
            
            pugi::xml_node sub_node = node.append_child("error");
            sub_node.append_attribute("id") = resp.transct_id;
            sub_node.append_child(pugi::node_pcdata).set_value("Account ID doesn't exist.");        
        }
        if (resp.type == "query")
        {
            pugi::xml_node node = root.append_child("status");
            node.append_attribute("id") = resp.transct_id;
            
            pugi::xml_node sub_node = node.append_child("error");
            sub_node.append_attribute("id") = resp.transct_id;
            sub_node.append_child(pugi::node_pcdata).set_value("Account ID doesn't exist.");        
        }
    }
    
    // Serialize the XML document to a string
    std::stringstream strs;
    doc.save(strs);

    // Print the generated XML text
    return strs.str();
}

std::string generate_trxn_resp(
    std::vector<transct> responses
)
{
    pugi::xml_document doc;

    // Add a root element to the document
    pugi::xml_node root = doc.append_child("results");
    for (auto resp : responses)
    {
        if (resp.type == "order")
        {
            pugi::xml_node node;
            node.append_attribute("sym") = resp.sym.c_str();
            node.append_attribute("amount") = resp.amount.c_str();
            node.append_attribute("limit") = resp.limit.c_str();
            if(resp.error_msg == ""){
                node = root.append_child("opened");
                node.append_attribute("id") = resp.transct_id;
            }
            else{
                node = root.append_child("error");
                node.append_child(pugi::node_pcdata).set_value("Account ID doesn't exist.");
            }
        }
        if (resp.type == "cancel")
        {
            pugi::xml_node node = root.append_child("canceled");
            node.append_attribute("id") = resp.transct_id;
            if (resp.error_msg != "")
            {
                pugi::xml_node sub_node = node.append_child("error");
                sub_node.append_attribute("id") = resp.transct_id;
                sub_node.append_child(pugi::node_pcdata).set_value(resp.error_msg.c_str());  
            }
            else{
                pugi::xml_node sub_node = node.append_child("canceled");
                sub_node.append_attribute("shares") = resp.cr.cancled_shares.c_str();
                sub_node.append_attribute("time") = resp.cr.cancled_time.c_str(); 
                for (auto exe : resp.cr.exes)
                {
                    pugi::xml_node exe_node = node.append_child("executed");
                    exe_node.append_attribute("shares") = exe.exe_shares.c_str();
                    exe_node.append_attribute("price") = exe.exe_price.c_str();
                    exe_node.append_attribute("time") = exe.exe_time.c_str();
                }
                
            }       
        }
        if (resp.type == "query")
        {
            pugi::xml_node node = root.append_child("status");
            node.append_attribute("id") = resp.transct_id;
            
            if (resp.error_msg != "")
            {
                pugi::xml_node sub_node = node.append_child("error");
                sub_node.append_attribute("id") = resp.transct_id;
                sub_node.append_child(pugi::node_pcdata).set_value(resp.error_msg.c_str());  
            }
            else{
                if(resp.qr.open_shares != ""){
                    pugi::xml_node sub_node = node.append_child("open");
                    sub_node.append_attribute("shares") = resp.qr.open_shares.c_str();
                }
                if(resp.qr.cancled_shares != ""){
                    pugi::xml_node sub_node = node.append_child("canceled");
                    sub_node.append_attribute("shares") = resp.qr.cancled_shares.c_str();
                    sub_node.append_attribute("time") = resp.qr.cancled_time.c_str();
                }
                for (auto exe : resp.qr.exes)
                {
                    pugi::xml_node exe_node = node.append_child("executed");
                    exe_node.append_attribute("shares") = exe.exe_shares.c_str();
                    exe_node.append_attribute("price") = exe.exe_price.c_str();
                    exe_node.append_attribute("time") = exe.exe_time.c_str();
                }
                
            } 
        }
    }
    
    // Serialize the XML document to a string
    std::stringstream strs;
    doc.save(strs);

    // Print the generated XML text
    return strs.str();
}