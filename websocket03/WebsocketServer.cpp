//
//  WebsocketServer.cpp
//  websocket03
//
//  Created by Claus Guttesen on 28/12/2015.
//  Copyright © 2015 Claus Guttesen. All rights reserved.
//

#include "WebsocketServer.hpp"

WebsocketServer::WebsocketServer() : m_next_session_id(1) {
    m_server.init_asio();
    
    m_server.set_open_handler(bind(&WebsocketServer::on_open, this, ::_1));
    m_server.set_close_handler(bind(&WebsocketServer::on_close, this, ::_1));
    m_server.set_message_handler(bind(&WebsocketServer::on_message, this, ::_1, ::_2, database));
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

void WebsocketServer::on_message(connection_hdl hdl, server::message_ptr msg, Database database) {
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
        if (jdata["type"] == "get table") {
            nlohmann::json message;

            // Get table
            message.clear();
            message = database.get_table(jdata);
            msg->set_payload(message.dump());
            m_server.send(hdl, msg);
            
            // Get matches
            message.clear();
            message = database.get_matches(jdata);
            msg->set_payload(message.dump());
            m_server.send(hdl, msg);
            
            // Get finished matches
            message.clear();
            message = database.get_finished_matches(jdata);
            msg->set_payload(message.dump());
            m_server.send(hdl, msg);
            
            // Get coming matches
            message.clear();
            message = database.get_coming_matches(jdata);
            msg->set_payload(message.dump());
            m_server.send(hdl, msg);
        }
        
        // Goal scored
        if (jdata["type"] == "goal") {
            nlohmann::json table;
            nlohmann::json matches;
            
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
                jdata["points"] = two;
                jdata["team"] = jdata["hometeam"];
                jdata["won"] = one;
                jdata["draw"] = minusone;
                jdata["lost"] = zero;
                database.update_standing(jdata);

                jdata["points"] = minusone;
                jdata["team"] = jdata["awayteam"];
                jdata["won"] = zero;
                jdata["draw"] = minusone;
                jdata["lost"] = one;
                database.update_standing(jdata);
            }
            // If it was equal score and awayteam score. And shuffle points.
            if (hts == ats && jdata["scoringteam"] == "awayteam") {
                // Subtract one point from the hometeam and add two to the awayteam.
                jdata["points"] = minusone;
                jdata["team"] = jdata["hometeam"];
                jdata["won"] = zero;
                jdata["draw"] = minusone;
                jdata["lost"] = one;
                database.update_standing(jdata);

                jdata["points"] = two;
                jdata["team"] = jdata["awayteam"];
                jdata["won"] = one;
                jdata["draw"] = minusone;
                jdata["lost"] = zero;
                database.update_standing(jdata);
            }
            
            // If hometeam is down by one and scores shuffle points.
            if ((hts - ats) == -1 && jdata["scoringteam"] == "hometeam") {
                // Add one point to hometeam and subtract two points from awayteam.
                jdata["points"] = one;
                jdata["team"] = jdata["hometeam"];
                jdata["won"] = zero;
                jdata["draw"] = one;
                jdata["lost"] = minusone;
                database.update_standing(jdata);

                jdata["points"] = minustwo;
                jdata["team"] = jdata["awayteam"];
                jdata["won"] = minusone;
                jdata["draw"] = one;
                jdata["lost"] = zero;
                database.update_standing(jdata);
            }
            
            // If awayteam is down by one and scores shuffle points.
            if ((hts - ats) == 1 && jdata["scoringteam"] == "awayteam") {
                // Subtract two points from hometeam and add one point to awayteam.
                jdata["points"] = minustwo;
                jdata["team"] = jdata["hometeam"];
                jdata["won"] = minusone;
                jdata["draw"] = one;
                jdata["lost"] = zero;
                database.update_standing(jdata);

                jdata["points"] = one;
                jdata["team"] = jdata["awayteam"];
                jdata["won"] = zero;
                jdata["draw"] = one;
                jdata["lost"] = minusone;
                database.update_standing(jdata);
            }
            
            // Add goal to matches- and teams-table.
            if (jdata["scoringteam"] == "hometeam") {
                jdata["team_score"] = "hometeam_score";
                database.update_goalscore(jdata);
            } else {
                jdata["team_score"] = "awayteam_score";
                database.update_goalscore(jdata);
            }
            
            // Get table
            table.clear();
            table = database.get_table(jdata);

            // Get matches
            matches.clear();
            matches = database.get_matches(jdata);
            
            // Broadcast to clients
            for (auto it : m_connections) {
                msg->set_payload(table.dump());
                m_server.send(it, msg);
                
                msg->set_payload(matches.dump());
                m_server.send(it, msg);
            }
        }
        
        // Goal canceled
        if (jdata["type"] == "cancelgoal") {
            nlohmann::json table;
            nlohmann::json matches;
            
            unsigned int hts = jdata["hometeam_score"];
            unsigned int ats = jdata["awayteam_score"];
            const std::string minusone = "-1";
            const std::string minustwo = "-2";
            const std::string zero = "0";
            const std::string one = "1";
            const std::string two = "2";
            
            // Don't cancel goal if score is zero.
            if ((jdata["scoringteam"] == "hometeam" && hts > 0) || (jdata["scoringteam"] == "awayteam" && ats > 0)) {
                // If it was equal score and hometeam have a goal cancelled. Shuffle points.
                if (hts == ats && jdata["scoringteam"] == "hometeam") {
                    // Subtract one point from the hometeam. And add two to the awayteam.
                    jdata["points"] = minusone;
                    jdata["team"] = jdata["hometeam"];
                    jdata["won"] = zero;
                    jdata["draw"] = minusone;
                    jdata["lost"] = one;
                    database.update_standing(jdata);

                    jdata["points"] = two;
                    jdata["team"] = jdata["awayteam"];
                    jdata["won"] = one;
                    jdata["draw"] = minusone;
                    jdata["lost"] = zero;
                    database.update_standing(jdata);
                }
                // If it was equal score and awayteam have a goal cancellled. Shuffle points.
                if (hts == ats && jdata["scoringteam"] == "awayteam") {
                    // Add two points to the hometeam and subtract one from the awayteam.
                    jdata["points"] = two;
                    jdata["team"] = jdata["hometeam"];
                    jdata["won"] = one;
                    jdata["draw"] = minusone;
                    jdata["lost"] = zero;
                    database.update_standing(jdata);

                    jdata["points"] = minusone;
                    jdata["team"] = jdata["awayteam"];
                    jdata["won"] = zero;
                    jdata["draw"] = minusone;
                    jdata["lost"] = zero;
                    database.update_standing(jdata);
                }
                
                // If hometeam is up by one and goal is cancelled shuffle points.
                if ((hts - ats) == 1 && jdata["scoringteam"] == "hometeam") {
                    // Subtract two points from hometeam and add one point to awayteam.
                    jdata["points"] = minustwo;
                    jdata["team"] = jdata["hometeam"];
                    jdata["won"] = minusone;
                    jdata["draw"] = one;
                    jdata["lost"] = zero;
                    database.update_standing(jdata);

                    jdata["points"] = one;
                    jdata["team"] = jdata["awayteam"];
                    jdata["won"] = zero;
                    jdata["draw"] = one;
                    jdata["lost"] = minusone;
                    database.update_standing(jdata);
                }
                
                // If awayteam is up by one and goal is cancelled shuffle points.
                if ((hts - ats) == -1 && jdata["scoringteam"] == "awayteam") {
                    // Add one point to hometeam and subtract two points from awayteam.
                    jdata["points"] = one;
                    jdata["team"] = jdata["hometeam"];
                    jdata["won"] = zero;
                    jdata["draw"] = one;
                    jdata["lost"] = minusone;
                    database.update_standing(jdata);

                    jdata["points"] = minustwo;
                    jdata["team"] = jdata["awayteam"];
                    jdata["won"] = minusone;
                    jdata["draw"] = one;
                    jdata["lost"] = zero;
                    database.update_standing(jdata);
                }
                
                // Remove goal to matches- and teams-table.
                if (jdata["scoringteam"] == "hometeam") {
                    jdata["team_score"] = "hometeam_score";
                    database.update_goalscore(jdata);
                } else {
                    jdata["team_score"] = "awayteam_score";
                    database.update_goalscore(jdata);
                }
                
                // Get table
                table.clear();
                table = database.get_table(jdata);
                
                // Get matches
                matches.clear();
                matches = database.get_matches(jdata);
                
                // Broadcast to clients
                for (auto it : m_connections) {
                    msg->set_payload(table.dump());
                    m_server.send(it, msg);
                    
                    msg->set_payload(matches.dump());
                    m_server.send(it, msg);
                }
            }
            
        }
        
        // Start match
        if (jdata["type"] == "start_match") {
            nlohmann::json table;
            nlohmann::json matches;
            nlohmann::json coming_matches;
            const std::string zero = "0";
            const std::string one = "1";
            jdata["points"] = one;
            jdata["won"] = zero;
            jdata["draw"] = one;
            jdata["lost"] = zero;
            
            jdata["team"] = jdata["hometeam"];
            database.update_standing(jdata);

            jdata["team"] = jdata["awayteam"];
            database.update_standing(jdata);

            database.start_match(jdata);
            
            // Get table
            table.clear();
            table = database.get_table(jdata);
            
            // Get matches
            matches.clear();
            matches = database.get_matches(jdata);
            
            // Get coming matches
            coming_matches.clear();
            coming_matches = database.get_coming_matches(jdata);
            
            // Broadcast to clients
            for (auto it : m_connections) {
                msg->set_payload(table.dump());
                m_server.send(it, msg);
                
                msg->set_payload(matches.dump());
                m_server.send(it, msg);

                msg->set_payload(coming_matches.dump());
                m_server.send(it, msg);
            }
        }
        
        // End match
        if (jdata["type"] == "end_match") {
            nlohmann::json matches;
            nlohmann::json coming_matches;
            nlohmann::json finished_matches;
            
            database.end_match(jdata);
            
            // Get matches
            matches.clear();
            matches = database.get_matches(jdata);
            
            // Get coming matches
            coming_matches.clear();
            coming_matches = database.get_coming_matches(jdata);
            
            // Get finished matches
            finished_matches.clear();
            finished_matches = database.get_finished_matches(jdata);
            
            // Broadcast to clients
            for (auto it : m_connections) {
                msg->set_payload(matches.dump());
                m_server.send(it, msg);
                
                msg->set_payload(coming_matches.dump());
                m_server.send(it, msg);
                
                msg->set_payload(finished_matches.dump());
                m_server.send(it, msg);
            }
        }
        
        // Get coming matches
        if (jdata["type"] == "get_coming_matches") {
            nlohmann::json message;

            message.clear();
            message = database.get_coming_matches(jdata);
            
            msg->set_payload(message.dump());
            for (auto it : m_connections) {
                m_server.send(it, msg);
            }
        }
        
        // Get finished matches
        if (jdata["type"] == "get_finished_matches") {
            nlohmann::json message;
            
            message.clear();
            message = database.get_finished_matches(jdata);
            
            msg->set_payload(message.dump());
            for (auto it : m_connections) {
                m_server.send(it, msg);
            }
        }
        
        // Get matches with no startdate
        if (jdata["type"] == "get_matches_without_startdate") {
            nlohmann::json message;
            
            message.clear();
            message = database.get_matches_without_startdate(jdata);
            
            msg->set_payload(message.dump());
            for (auto it : m_connections) {
                m_server.send(it, msg);
            }
        }
        
        // Set matchdate
        if (jdata["type"] == "set_match_date") {
            nlohmann::json matches_without_startdate;
            nlohmann::json coming_matches;
            
            database.set_matchdate(jdata);

            // Get matches without startdate
            matches_without_startdate.clear();
            matches_without_startdate = database.get_matches_without_startdate(jdata);
            
            // Get coming matches
            coming_matches.clear();
            coming_matches = database.get_coming_matches(jdata);
            
            // Broadcast to clients
            for (auto it : m_connections) {
                msg->set_payload(matches_without_startdate.dump());
                m_server.send(it, msg);
                
                msg->set_payload(coming_matches.dump());
                m_server.send(it, msg);
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
