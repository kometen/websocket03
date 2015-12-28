//
//  main.cpp
//  websocket03
//
//  Created by Claus Guttesen on 21/10/2015.
//  Copyright © 2015 Claus Guttesen. All rights reserved.
//
//  www.zaphoyd.com/websocketpp/manual/common-patterns/storing-connection-specificsession-information
//

#include <iostream>
#include "WebsocketServer.hpp"

int main(int argc, const char * argv[]) {
    WebsocketServer server;
/*    server.get_table();
    server.get_coming_matches("La Liga", "2015/2016");
    server.get_matches();
    server.get_finished_matches("La Liga", "2015/2016");*/
    server.run(9002);
    return 0;
}
