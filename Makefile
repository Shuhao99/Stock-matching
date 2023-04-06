CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
LDFLAGS = -lpqxx -lpq -pthread

all: exchange_server

main.o: main.cpp xml_generator.h xml_parser.h database.h server.h socket.h pugixml/pugixml.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

server.o: server.cpp xml_generator.h xml_parser.h database.h socket.h server.h session.h pugixml/pugixml.hpp
	$(CXX) $(CXXFLAGS) -c server.cpp $(EXTRAFLAGS)

xml_parser.o: xml_parser.cpp xml_parser.h pugixml/pugixml.hpp
	$(CXX) $(CXXFLAGS) -c xml_parser.cpp $(EXTRAFLAGS)

xml_generator.o: xml_generator.cpp xml_parser.h xml_generator.h pugixml/pugixml.hpp
	$(CXX) $(CXXFLAGS) -c xml_generator.cpp $(EXTRAFLAGS)

socket.o: socket.cpp socket.h 
	$(CXX) $(CXXFLAGS) -c socket.cpp $(EXTRAFLAGS)

database.o: database.cpp database.h
	$(CXX) $(CXXFLAGS) -c database.cpp $(EXTRAFLAGS)

pugixml.o: pugixml/pugixml.cpp pugixml/pugixml.hpp pugixml/pugiconfig.hpp
	$(CXX) $(CXXFLAGS) -c pugixml/pugixml.cpp

exchange_server: main.o server.o xml_parser.o socket.o database.o pugixml.o xml_generator.o
	$(CXX) $(CXXFLAGS) -o exchange_server main.o server.o xml_parser.o socket.o database.o pugixml.o xml_generator.o $(LDFLAGS)
	# rm -f main.o server.o xml_parser.o socket.o database.o pugixml.o xml_generator.o

clean:
	rm -f *~ *.o exchange_server

clobber:
	rm -f *~ *.o