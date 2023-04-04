#ifndef XML_GEN_H
#define XML_GEN_H
#include "xml_parser.h"
#include <iostream>
#include <sstream>

std::string generate_create_resp(std::vector<create> responses);

std::string generate_wrong_acc_id_resp(std::vector<transct> responses);

#endif
