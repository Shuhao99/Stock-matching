#ifndef SESSION_H
#define SESSION_H
#include <cstdlib>
#include <string>

struct session
{
  int fd;
  std::string ip;
};

#endif