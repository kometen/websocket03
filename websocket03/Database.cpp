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
    const std::string prepared_table = "get_table";
    std::string league = json["league"];
    std::string season = json["season"];
    
    nlohmann::json table;
    table["teams"] = { };
    table["type"] = "table";

    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "select * from teams where league = $1 and season = $2";
    (*D).prepare(prepared_table, query);
    pqxx::nontransaction N(*D);
    pqxx::result R(N.prepared(prepared_table)(league)(season).exec());

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
    const std::string prepared_table = "get_matches";
    std::string league = json["league"];
    std::string season = json["season"];
    
    nlohmann::json table;
    table["type"] = "matches";

    auto *D = dbpool.top();
    dbpool.pop();

    std::string query = "select * from matches where league = $1 and season = $2";
    query += " and match_began_at is not null";
    query += " and match_ended_at is null";
    query += " order by match_start_at asc, id";
    (*D).prepare(prepared_table, query);
    pqxx::nontransaction N(*D);
    pqxx::result R(N.prepared(prepared_table)(league)(season).exec());
    
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
    const std::string prepared_table = "get_finished_matches";
    std::string league = json["league"];
    std::string season = json["season"];

    nlohmann::json table;
    table["type"] = "finished_matches";
    table["teams"] = { };

    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "select * from matches where league = $1 and season = $2";
    query += " and match_start_at is not null and match_began_at is not null and match_ended_at is not null";
    query += " order by match_ended_at desc limit 10";
    (*D).prepare(prepared_table, query);
    pqxx::nontransaction N(*D);
    pqxx::result R(N.prepared(prepared_table)(league)(season).exec());
    
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
    const std::string prepared_table = "get_coming_matches";
    std::string league = json["league"];
    std::string season = json["season"];
    
    nlohmann::json table;
    table["type"] = "coming_matches";
    table["teams"] = { };

    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "select * from matches where league = $1 and season = $2";
    query += " and match_start_at is not null and match_began_at is null and match_ended_at is null";
    query += " order by match_start_at, hometeam limit 10";
    (*D).prepare(prepared_table, query);
    pqxx::nontransaction N(*D);
    pqxx::result R(N.prepared(prepared_table)(league)(season).exec());
    
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
    const std::string prepared_table = "get_matches_without_startdate";
    std::string league = json["league"];
    std::string season = json["season"];
    
    nlohmann::json table;
    table["type"] = "matches_without_startdate";

    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "select * from matches where league = $1 and season = $2";
    query += " and match_start_at is null and match_began_at is null and match_ended_at is null";
    query += " order by hometeam, awayteam";
    (*D).prepare(prepared_table, query);
    pqxx::nontransaction N(*D);
    pqxx::result R(N.prepared(prepared_table)(league)(season).exec());

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
    const std::string prepared_table = "set_matchdate";

    auto *D = dbpool.top();
    dbpool.pop();

    std::string query = "update matches set match_start_at = $1";
    query += " where league = $2 and season = $3 and hometeam = $4 and awayteam = $5";
    (*D).prepare(prepared_table, query);
    pqxx::work W(*D);
    W.prepared(prepared_table)(match_start_at)(league)(season)(hometeam)(awayteam).exec();
    W.commit();
    
    dbpool.push(D);
}

void Database::update_standing(std::string points, std::string league, std::string season, std::string team, std::string won, std::string draw, std::string lost) {
    const std::string prepared_table = "update_standing";

    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "update teams set points = points + $1";
    query += ", won = won + $2, draw = draw + $3, lost = lost + $4";
    query += " where league = $5 and season = $6 and team = $7";
    (*D).prepare(prepared_table, query);
    pqxx::work W(*D);
    W.prepared(prepared_table)(points)(won)(draw)(lost)(league)(season)(team).exec();
    W.commit();

    dbpool.push(D);
}

void Database::update_goalscore(std::string league, std::string season, std::string team, std::string venue, std::string goal, std::string hometeam, std::string awayteam) {
    const std::string prepared_table_home = "update_goalscore_home";
    const std::string prepared_table_away = "update_goalscore_away";

    auto *D = dbpool.top();
    pqxx::work W(*D);
    dbpool.pop();
    
    std::string team_score = venue + "team_score";
    std::cout << "team_score: " << team_score << std::endl;
    
//    std::string query = "update matches set " + venue + "team_score = " + venue + "team_score + " + goal;
    std::string query = "update matches set " + team_score + " = " + team_score + " + $1";
    query += " where league = $2 and season = $3 and hometeam = $4 and awayteam = $5";
    if (team_score == "hometeam_score") {
        (*D).prepare(prepared_table_home, query);
        W.prepared(prepared_table_home)(goal)(league)(season)(hometeam)(awayteam).exec();
    }
    if (team_score == "awayteam_score") {
        (*D).prepare(prepared_table_away, query);
        W.prepared(prepared_table_away)(goal)(league)(season)(hometeam)(awayteam).exec();
    }
    
    const std::string prepared_table_home1 = "update_hometeam_goalscore1";
    const std::string prepared_table_home2 = "update_hometeam_goalscore2";
    const std::string prepared_table_away1 = "update_awayteam_goalscore1";
    const std::string prepared_table_away2 = "update_awayteam_goalscore2";
    if (venue == "home") {
        // Add to goals_for to hometeam
        // query = "update teams set goals_for = goals_for + " + goal;
        query = "update teams set goals_for = goals_for + $1";
        query += " where league = $2 and season = $3 and team = $4";
        (*D).prepare(prepared_table_home1, query);
        W.prepared(prepared_table_home1)(goal)(league)(season)(hometeam).exec();
        
        // Add to goals_against to awayteam
        //query = "update teams set goals_against = goals_against + " + goal;
        query = "update teams set goals_against = goals_against + $1";
        query += " where league = $2 and season = $3 and team = $4";
        (*D).prepare(prepared_table_away1, query);
        W.prepared(prepared_table_away1)(goal)(league)(season)(awayteam).exec();
    } else {
        // Add to goals_for to awayteam
        // query = "update teams set goals_for = goals_for + " + goal;
        query = "update teams set goals_for = goals_for + $1";
        query += " where league = $2 and season = $3 and team = $4";
        (*D).prepare(prepared_table_away2, query);
        W.prepared(prepared_table_away2)(goal)(league)(season)(awayteam).exec();
        
        // Add to goals_against to hometeam
        // query = "update teams set goals_against = goals_against + " + goal;
        query = "update teams set goals_against = goals_against + $1";
        query += " where league = $2 and season = $3 and team = $4";
        (*D).prepare(prepared_table_home2, query);
        W.prepared(prepared_table_home2)(goal)(league)(season)(hometeam).exec();
    }
    
    W.commit();

    dbpool.push(D);
}

void Database::start_match(std::string league, std::string season, std::string hometeam, std::string awayteam) {
    const std::string prepared_table = "start_match";

    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "update matches set match_began_at = now()";
    query += " where league = $1 and season = $2 and hometeam = $3 and awayteam = $4";
    (*D).prepare(prepared_table, query);
    pqxx::work W(*D);
    W.prepared(prepared_table)(league)(season)(hometeam)(awayteam).exec();
    W.commit();
    
    dbpool.push(D);
}

void Database::end_match(std::string league, std::string season, std::string hometeam, std::string awayteam) {
    const std::string prepared_table = "end_match";

    auto *D = dbpool.top();
    dbpool.pop();
    
    std::string query = "update matches set match_ended_at = now()";
    query += " where league = $1 and season = $2 and hometeam = $3 and awayteam = $4";
    (*D).prepare(prepared_table, query);
    pqxx::work W(*D);
    W.prepared(prepared_table)(league)(season)(hometeam)(awayteam).exec();
    W.commit();

    dbpool.push(D);
}
