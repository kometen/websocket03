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
    nlohmann::json coming_matches;
    nlohmann::json matches_without_startdate;
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
        
        coming_matches["type"] = "coming_matches";
        coming_matches["teams"] = { };
        
        matches_without_startdate["type"] = "matches_without_startdate";
        matches_without_startdate["teams"] = { };
        
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
                jdata["cnt"] = clientmsg.length();
                msg->set_payload(jdata.dump());
                m_server.send(hdl, msg);
            }
            
            // Show table
            if (jdata["type"] == "request") {
                show_table(hdl, msg);
                show_matches(hdl, msg);
                show_coming_matches(hdl, msg);
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
                unsigned int hts = jdata["hometeam_score"];
                unsigned int ats = jdata["awayteam_score"];
                std::string minusone = "-1";
                std::string minustwo = "-2";
                std::string zero = "0";
                std::string one = "1";
                std::string two = "2";
                
                // If it was equal score and hometeam score. And shuffle points.
                if (hts == ats && jdata["scoringteam"] == "hometeam") {
                    // Add two points to the hometeam. And subtract one from the awayteam.
                    update_standing(two, jdata["league"], jdata["season"], jdata["hometeam"], one, minusone, zero);
                    update_standing(minusone, jdata["league"], jdata["season"], jdata["awayteam"], zero, minusone, one);
                }
                // If it was equal score and awayteam score. And shuffle points.
                if (hts == ats && jdata["scoringteam"] == "awayteam") {
                    // Subtract one point from the hometeam and add two to the awayteam.
                    update_standing(minusone, jdata["league"], jdata["season"], jdata["hometeam"], zero, minusone, one);
                    update_standing(two, jdata["league"], jdata["season"], jdata["awayteam"], one, minusone, zero);
                }
                
                // If hometeam is down by one and scores shuffle points.
                if ((hts - ats) == -1 && jdata["scoringteam"] == "hometeam") {
                    // Add one point to hometeam and subtract two points from awayteam.
                    update_standing(one, jdata["league"], jdata["season"], jdata["hometeam"], zero, one, minusone);
                    update_standing(minustwo, jdata["league"], jdata["season"], jdata["awayteam"], minusone, one, zero);
                }
                
                // If awayteam is down by one and scores shuffle points.
                if ((hts - ats) == 1 && jdata["scoringteam"] == "awayteam") {
                    // Subtract two points from hometeam and add one point to awayteam.
                    update_standing(minustwo, jdata["league"], jdata["season"], jdata["hometeam"], minusone, one, zero);
                    update_standing(one, jdata["league"], jdata["season"], jdata["awayteam"], zero, one, minusone);
                }

                // Add goal to matches- and teams-table.
                if (jdata["scoringteam"] == "hometeam") {
                    update_goalscore(jdata["league"], jdata["season"], jdata["hometeam"], "home", "1", jdata["hometeam"], jdata["awayteam"]);
                } else {
                    update_goalscore(jdata["league"], jdata["season"], jdata["awayteam"], "away", "1", jdata["hometeam"], jdata["awayteam"]);
                }

                get_table();
                get_matches();
                for (auto it : m_connections) {
                    show_table(it, msg);
                    show_matches(it, msg);
                }
            }
            
            // Start match
            if (jdata["type"] == "start_match") {
                std::string zero = "0";
                std::string one = "1";
                update_standing(one, jdata["league"], jdata["season"], jdata["hometeam"], zero, one, zero);
                update_standing(one, jdata["league"], jdata["season"], jdata["awayteam"], zero, one, zero);
                start_match(jdata["league"], jdata["season"], jdata["hometeam"], jdata["awayteam"]);

                get_table();
                get_matches();
                get_coming_matches(jdata["league"], jdata["season"]);
                for (auto it : m_connections) {
                    show_table(it, msg);
                    show_matches(it, msg);
                    show_coming_matches(it, msg);
                }
            }
            
            // End match
            if (jdata["type"] == "end_match") {
                end_match(jdata["league"], jdata["season"], jdata["hometeam"], jdata["awayteam"]);
                
                get_matches();
                get_coming_matches(jdata["league"], jdata["season"]);
                for (auto it : m_connections) {
                    show_matches(it, msg);
                    show_coming_matches(it, msg);
                }
            }
            
            // Get coming matches
            if (jdata["type"] == "fetch_coming_matches") {
                get_coming_matches(jdata["league"], jdata["season"]);
                for (auto it : m_connections) {
                    show_coming_matches(it, msg);
                }
            }
            
            // Get matches with no startdate
            if (jdata["type"] == "fetch_matches_without_startdate") {
                get_matches_without_startdate(jdata["league"], jdata["season"]);
                for (auto it : m_connections) {
                    show_matches_without_startdate(it, msg);
                }
            }

            // Set matchdate
            if (jdata["type"] == "set_match_date") {
                set_matchdate(jdata["id"], jdata["league"], jdata["season"], jdata["hometeam"], jdata["awayteam"], jdata["match_start_at"]);
                get_matches_without_startdate(jdata["league"], jdata["season"]);
                for (auto it : m_connections) {
                    show_matches_without_startdate(it, msg);
                }
                get_coming_matches(jdata["league"], jdata["season"]);
                for (auto it : m_connections) {
                    show_coming_matches(it, msg);
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
            table["teams"] = { };
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
                table["teams"] += { \
                    {"league", c[1].as<std::string>()}, \
                    {"season", c[2].as<std::string>()}, \
                    {"team", c[3].as<std::string>()}, \
                    {"points", c[4].as<int>()}, \
                    {"won", c[5].as<int>()}, \
                    {"draw", c[6].as<int>()}, \
                    {"lost", c[7].as<int>()}, \
                    {"goals_for", c[8].as<int>()}, \
                    {"goals_against", c[9].as<int>()} \
                };
                // Add team to unordered map to perform lookup when updating standings.
                teams_um.emplace(c[2].as<std::string>(), ++im);
            }
            C.disconnect();
            
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    
    void get_coming_matches(std::string league, std::string season) {
        try {
            coming_matches["teams"] = { };
            pqxx::connection C("dbname=sports user=claus hostaddr=127.0.0.1 port=5432");
            if (C.is_open()) {
                std::cout << "Connected to database" << std::endl;
            } else {
                std::cout << "Unable to connect to database" << std::endl;
            }
            std::string query = "";
            query = "select * from matches where league = '" + league + "'";
            query += " and season = '" + season + "'";
            query += " and match_start_at is not null and match_began_at is null and match_ended_at is null";
            query += " order by match_start_at asc limit 5";
            pqxx::nontransaction N(C);
            pqxx::result R(N.exec(query));
            for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
                coming_matches["teams"] += { \
                    {"id", c[0].as<int>()}, \
                    {"league", c[1].as<std::string>()}, \
                    {"season", c[2].as<std::string>()}, \
                    {"hometeam", c[3].as<std::string>()}, \
                    {"awayteam", c[4].as<std::string>()}, \
                    {"match_start_at", c[5].as<std::string>()}, \
                    {"hometeam_score", c[8].as<int>()}, \
                    {"awayteam_score", c[9].as<int>()} \
                };
            }
            C.disconnect();
            
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void set_matchdate(unsigned int id, std::string league, std::string season, std::string hometeam, std::string awayteam, std::string match_start_at) {
        try {
            pqxx::connection C("dbname=sports user=claus hostaddr=127.0.0.1 port=5432");
            if (C.is_open()) {
                //std::cout << "Connected to database" << std::endl;
            } else {
                std::cout << "Unable to connect to database" << std::endl;
            }
            std::string query = "";
            pqxx::work W(C);
            
            query = "update matches set match_start_at = '" + match_start_at + "'";
            query += " where league = '" + league + "'";
            query += " and season = '" + season + "'";
            query += " and hometeam = '" + hometeam + "'";
            query += " and awayteam = '" + awayteam + "'";
            
            std::cout << "update match with date: " << query << std::endl;
            
            W.exec(query);
            W.commit();
            C.disconnect();
            
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    
    void get_matches_without_startdate(std::string league, std::string season) {
        try {
            matches_without_startdate["teams"] = { };
            pqxx::connection C("dbname=sports user=claus hostaddr=127.0.0.1 port=5432");
            if (C.is_open()) {
                std::cout << "Connected to database" << std::endl;
            } else {
                std::cout << "Unable to connect to database" << std::endl;
            }
            std::string query = "";
            query = "select * from matches where league = '" + league + "'";
            query += " and season = '" + season + "'";
            query += " and match_start_at is null and match_began_at is null and match_ended_at is null";
            query += " order by hometeam, awayteam";
            pqxx::nontransaction N(C);
            pqxx::result R(N.exec(query));
            for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
                matches_without_startdate["teams"] += { \
                    {"id", c[0].as<int>()}, \
                    {"league", c[1].as<std::string>()}, \
                    {"season", c[2].as<std::string>()}, \
                    {"hometeam", c[3].as<std::string>()}, \
                    {"awayteam", c[4].as<std::string>()} \
                };
            }
            C.disconnect();
            
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    
    void get_matches() {
        try {
            matches["teams"] = { };
            pqxx::connection C("dbname=sports user=claus hostaddr=127.0.0.1 port=5432");
            if (C.is_open()) {
                std::cout << "Connected to database" << std::endl;
            } else {
                std::cout << "Unable to connect to database" << std::endl;
            }
            sql = "select * from matches where league = 'La Liga' \
            and season = '2015/2016' \
            and match_began_at is not null \
            and match_ended_at is null \
            order by match_start_at asc limit 5";
            pqxx::nontransaction N(C);
            pqxx::result R(N.exec(sql));
            for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
                matches["teams"] += { \
                    {"id", c[0].as<int>()}, \
                    {"league", c[1].as<std::string>()}, \
                    {"season", c[2].as<std::string>()}, \
                    {"hometeam", c[3].as<std::string>()}, \
                    {"awayteam", c[4].as<std::string>()}, \
                    {"match_start_at", c[5].as<std::string>()}, \
                    {"hometeam_score", c[8].as<int>()}, \
                    {"awayteam_score", c[9].as<int>()} \
                };
            }
            C.disconnect();
            
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    
    void update_standing(std::string points, std::string league, std::string season, std::string team, std::string won, std::string draw, std::string lost) {
        try {
            pqxx::connection C("dbname=sports user=claus hostaddr=127.0.0.1 port=5432");
            if (C.is_open()) {
//                std::cout << "Connected to database" << std::endl;
            } else {
                std::cout << "Unable to connect to database" << std::endl;
            }

            std::string query = "";
            pqxx::work W(C);

            query = "update teams set points = points + " + points;
            query += ", won = won + " + won;
            query += ", draw = draw + " + draw;
            query += ", lost = lost + " + lost;
            query += " where league = '" + league;
            query += "' and season = '" + season;
            query += "' and team = '" + team + "'";

            W.exec(query);
            W.commit();
            C.disconnect();
            
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    
    void update_goalscore(std::string league, std::string season, std::string team, std::string venue, std::string goal, std::string hometeam, std::string awayteam) {
        try {
            pqxx::connection C("dbname=sports user=claus hostaddr=127.0.0.1 port=5432");
            if (C.is_open()) {
                //                std::cout << "Connected to database" << std::endl;
            } else {
                std::cout << "Unable to connect to database" << std::endl;
            }
            
            std::string query = "";
            pqxx::work W(C);
            
            query = "update matches set " + venue + "team_score = " + venue + "team_score + " + goal;
            query += " where league = '" + league;
            query += "' and season = '" + season;
            query += "' and " + venue + "team = '" + team + "'";

            W.exec(query);
            
            if (venue == "home") {
                // Add to goals_for to hometeam
                query = "update teams set goals_for = goals_for + " + goal;
                query += " where league = '" + league;
                query += "' and season = '" + season;
                query += "' and team = '" + hometeam + "'";
                
                W.exec(query);

                // Add to goals_against to awayteam
                query = "update teams set goals_against = goals_against + " + goal;
                query += " where league = '" + league;
                query += "' and season = '" + season;
                query += "' and team = '" + awayteam + "'";
                
                W.exec(query);
            } else {
                // Add to goals_for to awayteam
                query = "update teams set goals_for = goals_for + " + goal;
                query += " where league = '" + league;
                query += "' and season = '" + season;
                query += "' and team = '" + awayteam + "'";
                
                W.exec(query);

                // Add to goals_against to hometeam
                query = "update teams set goals_against = goals_against + " + goal;
                query += " where league = '" + league;
                query += "' and season = '" + season;
                query += "' and team = '" + hometeam + "'";
                
                W.exec(query);
            }
            
            W.commit();
            C.disconnect();
            
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    
    void start_match(std::string league, std::string season, std::string hometeam, std::string awayteam) {
        try {
            pqxx::connection C("dbname=sports user=claus hostaddr=127.0.0.1 port=5432");
            if (C.is_open()) {
                //                std::cout << "Connected to database" << std::endl;
            } else {
                std::cout << "Unable to connect to database" << std::endl;
            }
            
            std::string query = "";
            pqxx::work W(C);
            
            query = "update matches set match_began_at = now()";
            query += " where league = '" + league;
            query += "' and season = '" + season;
            query += "' and hometeam = '" + hometeam;
            query += "' and awayteam = '" + awayteam + "'";
            
            W.exec(query);
            W.commit();
            C.disconnect();
            
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    
    void end_match(std::string league, std::string season, std::string hometeam, std::string awayteam) {
        try {
            pqxx::connection C("dbname=sports user=claus hostaddr=127.0.0.1 port=5432");
            if (C.is_open()) {
                //                std::cout << "Connected to database" << std::endl;
            } else {
                std::cout << "Unable to connect to database" << std::endl;
            }
            
            std::string query = "";
            pqxx::work W(C);
            
            query = "update matches set match_ended_at = now()";
            query += " where league = '" + league;
            query += "' and season = '" + season;
            query += "' and hometeam = '" + hometeam;
            query += "' and awayteam = '" + awayteam + "'";
            
            W.exec(query);
            W.commit();
            C.disconnect();
            
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    
    void show_table(connection_hdl hdl, server::message_ptr msg) {
        msg->set_payload(table.dump());
        m_server.send(hdl, msg);
    }
    
    void show_matches(connection_hdl hdl, server::message_ptr msg) {
        msg->set_payload(matches.dump());
        m_server.send(hdl, msg);
    }

    void show_coming_matches(connection_hdl hdl, server::message_ptr msg) {
        msg->set_payload(coming_matches.dump());
        m_server.send(hdl, msg);
    }
    
    void show_matches_without_startdate(connection_hdl hdl, server::message_ptr msg) {
        msg->set_payload(matches_without_startdate.dump());
        m_server.send(hdl, msg);
    }
    
};

int main(int argc, const char * argv[]) {
    print_server server;
    server.get_table();
    server.get_coming_matches("La Liga", "2015/2016");
    server.get_matches();
    server.run(9002);
    return 0;
}
