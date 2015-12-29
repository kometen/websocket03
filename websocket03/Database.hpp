//
//  Database.hpp
//  websocket03
//
//  Created by Claus Guttesen on 28/12/2015.
//  Copyright © 2015 Claus Guttesen. All rights reserved.
//

#ifndef Database_hpp
#define Database_hpp

#include <json.hpp>
#include <pqxx/pqxx>

class Database {
private:

public:
    nlohmann::json get_table(const nlohmann::json);
    nlohmann::json get_matches(const nlohmann::json);
    nlohmann::json get_finished_matches(const nlohmann::json);
    nlohmann::json get_coming_matches(const nlohmann::json);
    nlohmann::json get_matches_without_startdate(const nlohmann::json);
    void set_matchdate(unsigned int id, std::string league, std::string season, std::string hometeam, std::string awayteam, std::string match_start_at);
    void update_standing(std::string points, std::string league, std::string season, std::string team, std::string won, std::string draw, std::string lost);
    void update_goalscore(std::string league, std::string season, std::string team, std::string venue, std::string goal, std::string hometeam, std::string awayteam);
    void start_match(std::string league, std::string season, std::string hometeam, std::string awayteam);
    void end_match(std::string league, std::string season, std::string hometeam, std::string awayteam);
};

#endif /* Database_hpp */
