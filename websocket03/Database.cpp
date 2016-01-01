//
//  Database.cpp
//  websocket03
//
//  Created by Claus Guttesen on 28/12/2015.
//  Copyright © 2015 Claus Guttesen. All rights reserved.
//

#include "Database.hpp"

Database::Database(const unsigned int connections) {
    for (int i = 0; i < connections; ++i) {
        try {
            auto* dbconn = new pqxx::connection(connectionString);
            dbpool.emplace(dbconn);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

nlohmann::json Database::get_table(const nlohmann::json json) {
    std::string league = json["league"];
    std::string season = json["season"];
    
    nlohmann::json table;
    table["teams"] = { };
    table["type"] = "table";

    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "select * from teams where league = '" + league + "' and season = '" + season + "'";
    pqxx::nontransaction N(*D);
    pqxx::result R(N.exec(query));
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
    }

    dbpool.push(D);
    
    return table;
}

nlohmann::json Database::get_matches(const nlohmann::json json) {
    std::string league = json["league"];
    std::string season = json["season"];
    
    nlohmann::json table;
    table["matches"] = { };
    table["type"] = "matches";

    auto *D = dbpool.top();
    dbpool.pop();

    std::string query = "select * from matches where league = '" + league + "' \
    and season = '" + season + "' \
    and match_began_at is not null \
    and match_ended_at is null \
    order by match_start_at asc, id";
    pqxx::nontransaction N(*D);
    pqxx::result R(N.exec(query));
    for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
        table["teams"] += { \
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
    
    dbpool.push(D);
    
    return table;
}

nlohmann::json Database::get_finished_matches(const nlohmann::json json) {
    std::string league = json["league"];
    std::string season = json["season"];

    nlohmann::json table;
    table["matches"] = { };
    table["type"] = "finished_matches";
    table["teams"] = { };

    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "select * from matches where league = '" + league + "'";
    query += " and season = '" + season + "'";
    query += " and match_start_at is not null and match_began_at is not null and match_ended_at is not null";
    query += " order by match_ended_at desc limit 10";
    pqxx::nontransaction N(*D);
    pqxx::result R(N.exec(query));
    for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
        table["teams"] += { \
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
    
    dbpool.push(D);
    
    return table;
}

nlohmann::json Database::get_coming_matches(const nlohmann::json json) {
    std::string league = json["league"];
    std::string season = json["season"];
    
    nlohmann::json table;
    table["matches"] = { };
    table["type"] = "coming_matches";
    table["teams"] = { };

    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "select * from matches where league = '" + league + "'";
    query += " and season = '" + season + "'";
    query += " and match_start_at is not null and match_began_at is null and match_ended_at is null";
    query += " order by match_start_at, hometeam limit 10";
    pqxx::nontransaction N(*D);
    pqxx::result R(N.exec(query));
    for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
        table["teams"] += { \
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
    
    dbpool.push(D);
    
    return table;
}

nlohmann::json Database::get_matches_without_startdate(const nlohmann::json json) {
    std::string league = json["league"];
    std::string season = json["season"];
    
    nlohmann::json table;
    table["teams"] = { };
    table["type"] = "matches_without_startdate";

    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "select * from matches where league = '" + league + "'";
    query += " and season = '" + season + "'";
    query += " and match_start_at is null and match_began_at is null and match_ended_at is null";
    query += " order by hometeam, awayteam";
    pqxx::nontransaction N(*D);
    pqxx::result R(N.exec(query));
    for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
        table["teams"] += { \
            {"id", c[0].as<int>()}, \
            {"league", c[1].as<std::string>()}, \
            {"season", c[2].as<std::string>()}, \
            {"hometeam", c[3].as<std::string>()}, \
            {"awayteam", c[4].as<std::string>()} \
        };
    }
    
    dbpool.push(D);
    
    return table;
}

void Database::set_matchdate(unsigned int id, std::string league, std::string season, std::string hometeam, std::string awayteam, std::string match_start_at) {
    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "";
    pqxx::work W(*D);
    
    query = "update matches set match_start_at = '" + match_start_at + "'";
    query += " where league = '" + league + "'";
    query += " and season = '" + season + "'";
    query += " and hometeam = '" + hometeam + "'";
    query += " and awayteam = '" + awayteam + "'";
    
    std::cout << "update match with date: " << query << std::endl;
    
    W.exec(query);
    W.commit();
    
    dbpool.push(D);
}

void Database::update_standing(std::string points, std::string league, std::string season, std::string team, std::string won, std::string draw, std::string lost) {
    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "";
    pqxx::work W(*D);
    
    query = "update teams set points = points + " + points;
    query += ", won = won + " + won;
    query += ", draw = draw + " + draw;
    query += ", lost = lost + " + lost;
    query += " where league = '" + league;
    query += "' and season = '" + season;
    query += "' and team = '" + team + "'";
    
    W.exec(query);
    W.commit();

    dbpool.push(D);
}

void Database::update_goalscore(std::string league, std::string season, std::string team, std::string venue, std::string goal, std::string hometeam, std::string awayteam) {
    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "";
    pqxx::work W(*D);
    
    query = "update matches set " + venue + "team_score = " + venue + "team_score + " + goal;
    query += " where league = '" + league;
    query += "' and season = '" + season;
    query += "' and hometeam = '" + hometeam;
    query += "' and awayteam = '" + awayteam + "'";
    
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

    dbpool.push(D);
}

void Database::start_match(std::string league, std::string season, std::string hometeam, std::string awayteam) {
    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "";
    pqxx::work W(*D);
    
    query = "update matches set match_began_at = now()";
    query += " where league = '" + league;
    query += "' and season = '" + season;
    query += "' and hometeam = '" + hometeam;
    query += "' and awayteam = '" + awayteam + "'";
    
    W.exec(query);
    W.commit();
    
    dbpool.push(D);
}

void Database::end_match(std::string league, std::string season, std::string hometeam, std::string awayteam) {
    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "";
    pqxx::work W(*D);
    
    query = "update matches set match_ended_at = now()";
    query += " where league = '" + league;
    query += "' and season = '" + season;
    query += "' and hometeam = '" + hometeam;
    query += "' and awayteam = '" + awayteam + "'";
    
    W.exec(query);
    W.commit();

    dbpool.push(D);
}
