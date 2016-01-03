//
//  Database.hpp
//  websocket03
//
//  Created by Claus Guttesen on 28/12/2015.
//  Copyright © 2015 Claus Guttesen. All rights reserved.
//

#ifndef Database_hpp
#define Database_hpp

#include <stack>
#include <json.hpp>
#include <pqxx/pqxx>

class Database {
private:
    const std::string connectionString = "dbname=sports user=claus hostaddr=127.0.0.1 port=5432";
    std::stack<pqxx::connection*> dbpool;

public:
    Database(const unsigned int);
    nlohmann::json get_table(const nlohmann::json);
    nlohmann::json get_matches(const nlohmann::json);
    nlohmann::json get_finished_matches(const nlohmann::json);
    nlohmann::json get_coming_matches(const nlohmann::json);
    nlohmann::json get_matches_without_startdate(const nlohmann::json);
    void set_matchdate(const nlohmann::json);
//    void update_standing(std::string points, std::string league, std::string season, std::string team, std::string won, std::string draw, std::string lost);
    void update_standing(const nlohmann::json);
    void update_goalscore(const nlohmann::json);
    void start_match(const nlohmann::json);
    void end_match(const nlohmann::json);
};

#endif /* Database_hpp */
