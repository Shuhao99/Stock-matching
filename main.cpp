#include "server.h"

int main() {
  const char * port = "12345";
  server * my_server = new server(port);
  my_server->run();
  return 0;
}