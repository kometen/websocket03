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
#include <unordered_map>
#include <pqxx/pqxx>
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
    nlohmann::json matches;
    std::unordered_map<std::string, unsigned int> teams_um;
    typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;
    con_list m_connections;
    const char* sql;
    
public:

    print_server() : m_next_session_id(1) {
        m_server.init_asio();
        
        m_server.set_open_handler(bind(&print_server::on_open, this, ::_1));
        m_server.set_close_handler(bind(&print_server::on_close, this, ::_1));
        m_server.set_message_handler(bind(&print_server::on_message, this, ::_1, ::_2));

        table["type"] = "table";
        table["teams"] = { };
        
        matches["type"] = "matches";
        matches["teams"] = { };
        
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
                msg->set_payload(matches.dump());
                m_server.send(hdl, msg);
            }
            
            // Update stats
            if (jdata["type"] == "update") {
                std::string t = jdata["team"];
                std::string p = jdata["points"];
                // If team is in unordered_map (reverse lookup).
                if (teams_um.count(t)) {
                    auto i = teams_um.find(t);
                    unsigned int points = table["teams"][i->second]["points"];
                    unsigned int given = std::stoi(p);
                    table["teams"][i->second]["points"] = (points + given);
                    for (auto it : m_connections) {
                        msg->set_payload(table.dump());
                        m_server.send(it, msg);
                    }
                }
                
            }

            // Goal scored
            if (jdata["type"] == "goal") {
                std::cout << "Goal scored by " << jdata["scoringteam"] << ", hometeam: " << jdata["hometeam"] << ", awayteam: " << jdata["awayteam"] << std::endl;

                // If it was equal score and hometeam score. Else ...
                if (jdata["hometeam_score"] == jdata["awayteam_score"] && jdata["scoringteam"] == "hometeam") {
                    std::cout << "Add two points to the hometeam and subtract one from the awayteam." << std::endl;
                } else {
                    std::cout << "Subtract one point from the hometeam and add two to the awayteam." << std::endl;
                }
                
                unsigned int hts = jdata["hometeam_score"];
                unsigned int ats = jdata["awayteam_score"];

                // If hometeam is down by one and scores.
                if ((hts - ats) == -1 && jdata["scoringteam"] == "hometeam") {
                    std::cout << "Add one point to hometeam and subtract two points from awayteam" << std::endl;
                }
                
                // If awayteam is down by one and scores.
                if ((hts - ats) == 1 && jdata["scoringteam"] == "awayteam") {
                    std::cout << "Subtract two points from hometeam and add one point to awayteam" << std::endl;
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
    
    void get_table() {
        try {
            pqxx::connection C("dbname=sports user=claus hostaddr=127.0.0.1 port=5432");
            if (C.is_open()) {
                std::cout << "Connected to database" << std::endl;
            } else {
                std::cout << "Unable to connect to database" << std::endl;
            }
            sql = "select * from teams where league = 'La Liga' and season = '2015/2016'";
            pqxx::nontransaction N(C);
            pqxx::result R(N.exec(sql));
            int im = -1;
            for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
                table["teams"] += { {"team", c[3].as<std::string>()}, {"points", c[4].as<int>()} };
                // Add team to unordered map to perform lookup when updating standings.
                teams_um.emplace(c[2].as<std::string>(), ++im);
            }
            C.disconnect();
            
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    
    void get_coming_matches() {
        try {
            pqxx::connection C("dbname=sports user=claus hostaddr=127.0.0.1 port=5432");
            if (C.is_open()) {
                std::cout << "Connected to database" << std::endl;
            } else {
                std::cout << "Unable to connect to database" << std::endl;
            }
            sql = "select * from matches where league = 'La Liga' \
                and season = '2015/2016' \
                and match_end_at is null \
                order by match_start_at asc limit 5";
            pqxx::nontransaction N(C);
            pqxx::result R(N.exec(sql));
            for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
                matches["teams"] += { \
                    {"id", c[0].as<int>()}, \
                    {"hometeam", c[3].as<std::string>()}, \
                    {"awayteam", c[4].as<std::string>()}, \
                    {"match_start_at", c[5].as<std::string>()}, \
                    {"hometeam_score", c[7].as<int>()}, \
                    {"awayteam_score", c[8].as<int>()} \
                };
            }
            C.disconnect();
            
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
};

int main(int argc, const char * argv[]) {
    print_server server;
    server.get_table();
    server.get_coming_matches();
    server.run(9002);
    return 0;
}
