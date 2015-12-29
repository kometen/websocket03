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
    server.run(9002);
    return 0;
}
