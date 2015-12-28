//
//  WebsocketServer.cpp
//  websocket03
//
//  Created by Claus Guttesen on 28/12/2015.
//  Copyright © 2015 Claus Guttesen. All rights reserved.
//

#include "WebsocketServer.hpp"

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

void show_finished_matches(connection_hdl hdl, server::message_ptr msg) {
    msg->set_payload(finished_matches.dump());
    m_server.send(hdl, msg);
}

void show_matches_without_startdate(connection_hdl hdl, server::message_ptr msg) {
    msg->set_payload(matches_without_startdate.dump());
    m_server.send(hdl, msg);
}

void WebsocketServer::on_open(connection_hdl hdl) {
    connection_ptr con = m_server.get_con_from_hdl(hdl);
    
    con->session_id = ++m_next_session_id;
    std::cout << "Opening connection with connection id " << con->session_id << std::endl;
    m_connections.insert(hdl);
}

void WebsocketServer::on_close(connection_hdl hdl) {
    connection_ptr con = m_server.get_con_from_hdl(hdl);
    
    std::cout << "Closing connection " << con->name << " with session id " << con->session_id << std::endl;
    m_connections.erase(hdl);
}

void WebsocketServer::on_message(connection_hdl hdl, server::message_ptr msg) {
    connection_ptr con = m_server.get_con_from_hdl(hdl);
    Database database {};
    
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
        if (jdata["type"] == "get table") {
            show_table(hdl, msg);
            show_matches(hdl, msg);
            show_finished_matches(hdl, msg);
            show_coming_matches(hdl, msg);
        }
        
        // Goal scored
        if (jdata["type"] == "goal") {
            unsigned int hts = jdata["hometeam_score"];
            unsigned int ats = jdata["awayteam_score"];
            int goal = jdata["goal"];
            std::string sgoal = std::to_string(goal);
            std::string minusone = "-1";
            std::string minustwo = "-2";
            std::string zero = "0";
            std::string one = "1";
            std::string two = "2";
            
            // If it was equal score and hometeam score. And shuffle points.
            if (hts == ats && jdata["scoringteam"] == "hometeam") {
                // Add two points to the hometeam. And subtract one from the awayteam.
                database.update_standing(two, jdata["league"], jdata["season"], jdata["hometeam"], one, minusone, zero);
                database.update_standing(minusone, jdata["league"], jdata["season"], jdata["awayteam"], zero, minusone, one);
            }
            // If it was equal score and awayteam score. And shuffle points.
            if (hts == ats && jdata["scoringteam"] == "awayteam") {
                // Subtract one point from the hometeam and add two to the awayteam.
                database.update_standing(minusone, jdata["league"], jdata["season"], jdata["hometeam"], zero, minusone, one);
                database.update_standing(two, jdata["league"], jdata["season"], jdata["awayteam"], one, minusone, zero);
            }
            
            // If hometeam is down by one and scores shuffle points.
            if ((hts - ats) == -1 && jdata["scoringteam"] == "hometeam") {
                // Add one point to hometeam and subtract two points from awayteam.
                database.update_standing(one, jdata["league"], jdata["season"], jdata["hometeam"], zero, one, minusone);
                database.update_standing(minustwo, jdata["league"], jdata["season"], jdata["awayteam"], minusone, one, zero);
            }
            
            // If awayteam is down by one and scores shuffle points.
            if ((hts - ats) == 1 && jdata["scoringteam"] == "awayteam") {
                // Subtract two points from hometeam and add one point to awayteam.
                database.update_standing(minustwo, jdata["league"], jdata["season"], jdata["hometeam"], minusone, one, zero);
                database.update_standing(one, jdata["league"], jdata["season"], jdata["awayteam"], zero, one, minusone);
            }
            
            // Add goal to matches- and teams-table.
            if (jdata["scoringteam"] == "hometeam") {
                database.update_goalscore(jdata["league"], jdata["season"], jdata["hometeam"], "home", sgoal, jdata["hometeam"], jdata["awayteam"]);
            } else {
                database.update_goalscore(jdata["league"], jdata["season"], jdata["awayteam"], "away", sgoal, jdata["hometeam"], jdata["awayteam"]);
            }
            
            get_table();
            get_matches();
            for (auto it : m_connections) {
                show_table(it, msg);
                show_matches(it, msg);
            }
        }
        
        // Goal canceled
        if (jdata["type"] == "cancelgoal") {
            unsigned int hts = jdata["hometeam_score"];
            unsigned int ats = jdata["awayteam_score"];
            int goal = jdata["goal"];
            std::string sgoal = std::to_string(goal);
            std::string minusone = "-1";
            std::string minustwo = "-2";
            std::string zero = "0";
            std::string one = "1";
            std::string two = "2";
            
            // Don't cancel goal if score is zero.
            if ((jdata["scoringteam"] == "hometeam" && hts > 0) || (jdata["scoringteam"] == "awayteam" && ats > 0)) {
                // If it was equal score and hometeam have a goal cancelled. Shuffle points.
                if (hts == ats && jdata["scoringteam"] == "hometeam") {
                    // Subtract one point from the hometeam. And add two to the awayteam.
                    update_standing(minusone, jdata["league"], jdata["season"], jdata["hometeam"], zero, minusone, one);
                    update_standing(two, jdata["league"], jdata["season"], jdata["awayteam"], one, minusone, zero);
                }
                // If it was equal score and awayteam have a goal cancellled. Shuffle points.
                if (hts == ats && jdata["scoringteam"] == "awayteam") {
                    // Add two points to the hometeam and subtract one from the awayteam.
                    update_standing(two, jdata["league"], jdata["season"], jdata["hometeam"], one, minusone, zero);
                    update_standing(minusone, jdata["league"], jdata["season"], jdata["awayteam"], zero, minusone, one);
                }
                
                // If hometeam is up by one and goal is cancelled shuffle points.
                if ((hts - ats) == 1 && jdata["scoringteam"] == "hometeam") {
                    // Subtract two points from hometeam and add one point to awayteam.
                    update_standing(minustwo, jdata["league"], jdata["season"], jdata["hometeam"], minusone, one, zero);
                    update_standing(one, jdata["league"], jdata["season"], jdata["awayteam"], zero, one, minusone);
                }
                
                // If awayteam is up by one and goal is cancelled shuffle points.
                if ((hts - ats) == -1 && jdata["scoringteam"] == "awayteam") {
                    // Add one point to hometeam and subtract two points from awayteam.
                    update_standing(one, jdata["league"], jdata["season"], jdata["hometeam"], zero, one, minusone);
                    update_standing(minustwo, jdata["league"], jdata["season"], jdata["awayteam"], minusone, one, zero);
                }
                
                // Remove goal to matches- and teams-table.
                if (jdata["scoringteam"] == "hometeam") {
                    update_goalscore(jdata["league"], jdata["season"], jdata["hometeam"], "home", sgoal, jdata["hometeam"], jdata["awayteam"]);
                } else {
                    update_goalscore(jdata["league"], jdata["season"], jdata["awayteam"], "away", sgoal, jdata["hometeam"], jdata["awayteam"]);
                }
                
                get_table();
                get_matches();
                for (auto it : m_connections) {
                    show_table(it, msg);
                    show_matches(it, msg);
                }
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
            get_finished_matches(jdata["league"], jdata["season"]);
            for (auto it : m_connections) {
                show_matches(it, msg);
                show_coming_matches(it, msg);
                show_finished_matches(it, msg);
            }
        }
        
        // Get coming matches
        if (jdata["type"] == "get_coming_matches") {
            get_coming_matches(jdata["league"], jdata["season"]);
            for (auto it : m_connections) {
                show_coming_matches(it, msg);
            }
        }
        
        // Get finished matches
        if (jdata["type"] == "get_finished_matches") {
            get_finished_matches(jdata["league"], jdata["season"]);
            for (auto it : m_connections) {
                show_finished_matches(it, msg);
            }
        }
        
        // Get matches with no startdate
        if (jdata["type"] == "get_matches_without_startdate") {
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

void WebsocketServer::run(uint16_t port) {
    m_server.listen(websocketpp::lib::asio::ip::tcp::v4(), port);
    m_server.start_accept();
    m_server.run();
}
