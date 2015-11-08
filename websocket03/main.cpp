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
#include <json.hpp>
#include <set>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

struct connection_data {
    unsigned int session_id;
    std::string name;
};

struct custom_config : public websocketpp::config::asio {
    // Pull default settings.
    typedef websocketpp::config::asio core;
    
    typedef core::concurrency_type concurrency_type;
    typedef core::request_type request_type;
    typedef core::response_type response_type;
    typedef core::message_type message_type;
    typedef core::con_msg_manager_type con_msg_manager_type;
    typedef core::endpoint_msg_manager_type endpoint_msg_manager_type;
    typedef core::alog_type alog_type;
    typedef core::elog_type elog_type;
    typedef core::rng_type rng_type;
    typedef core::transport_type transport_type;
    typedef core::endpoint_base endpoint_base;
    
    // Set a custom connection_base class.
    typedef connection_data connection_base;
};

typedef websocketpp::server<custom_config> server;
typedef server::connection_ptr connection_ptr;


using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

class print_server {
private:
    int connections = 0;
    unsigned int m_next_session_id;
    server m_server;
    nlohmann::json table;
    typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;
    con_list m_connections;
    
public:

    print_server() : m_next_session_id(1) {
        m_server.init_asio();
        
        m_server.set_open_handler(bind(&print_server::on_open, this, ::_1));
        m_server.set_close_handler(bind(&print_server::on_close, this, ::_1));
        m_server.set_message_handler(bind(&print_server::on_message, this, ::_1, ::_2));

        table["type"] = "table";
        table["teams"] = {
            {{"team", "Barcelona"}, {"points", 24}, {"stats", {8, 0, 2, 22, 12}}}, \
            {{"team", "Real Madrid"}, {"points", 24}, {"stats", {7, 3, 0, 24, 4}}}, \
            {{"team", "Celta Vigo"}, {"points", 21}, {"stats", {6, 3, 1 }}}
        };
    }
    
    void on_open(connection_hdl hdl) {
        connection_ptr con = m_server.get_con_from_hdl(hdl);
        
        con->session_id = ++m_next_session_id;
        std::cout << "Opening connection with connection id " << con->session_id << std::endl;
        m_connections.insert(hdl);
    }
    
    void on_close(connection_hdl hdl) {
        connection_ptr con = m_server.get_con_from_hdl(hdl);
        
        std::cout << "Closing connection " << con->name << " with session id " << con->session_id << std::endl;
        m_connections.erase(hdl);
    }
    
    void on_message(connection_hdl hdl, server::message_ptr msg) {
        connection_ptr con = m_server.get_con_from_hdl(hdl);

        nlohmann::json jdata;
        
        std::string payload = msg->get_payload();
        
        try {
            jdata.clear();
            jdata = nlohmann::json::parse(payload);
            
            if (jdata["type"] == "lgn") {
                std::string lname = jdata["data"];
                if (con->name == "") {
                    con->name = lname;
                    std::cout << "Setting name with session id " << con->session_id << " to " << lname << std::endl;
                } else {
                    std::cout << "Got message from connection " << lname << " with session id " << con->session_id << std::endl;
                }
            }
            
            if (jdata["type"] == "msg") {
                std::string clientmsg = jdata["data"];
                std::cout << "Message sent: " << clientmsg << std::endl;
                jdata["cnt"] = clientmsg.length();
                msg->set_payload(jdata.dump());
                m_server.send(hdl, msg);
            }
            
            // Show table
            if (jdata["type"] == "request") {
                msg->set_payload(table.dump());
                m_server.send(hdl, msg);
            }
            
            // Update stats
            if (jdata["type"] == "update") {
                std::string s = jdata["data"];
                unsigned int points = table["teams"][2]["points"];
                unsigned int given = std::stoi(s);
                table["teams"][2]["points"] = (points + given);
                for (auto it : m_connections) {
                    msg->set_payload(table.dump());
                    m_server.send(it, msg);
                }
            }
            
            
        } catch (const std::exception& e) {
            msg->set_payload("Unable to parse json");
            m_server.send(hdl, msg);
            std::cerr << "Unable to parse json: " << e.what() << std::endl;
        }
    }
    
    void run(uint16_t port) {
        m_server.listen(websocketpp::lib::asio::ip::tcp::v4(), port);
        m_server.start_accept();
        m_server.run();
    }
};

int main(int argc, const char * argv[]) {
    std::cout << "Hello, World!\n";
    print_server server;
    server.run(9002);
    return 0;
}
