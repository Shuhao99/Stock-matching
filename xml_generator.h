#ifndef XML_GEN_H
#define XML_GEN_H
#include "xml_parser.h"
#include <iostream>
#include <sstream>

struct query_res
{
    /* data */
};

struct cancel_res
{
    /* data */
};

std::string generate_create_resp(std::vector<create> responses);

#endif
