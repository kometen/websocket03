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
    nlohmann::json table;
    nlohmann::json coming_matches;
    nlohmann::json finished_matches;
    nlohmann::json matches_without_startdate;
    nlohmann::json matches;
    std::string query;

public:
    void get_table();
    void get_coming_matches(std::string league, std::string season);
    void get_finished_matches(std::string league, std::string season);
    void set_matchdate(unsigned int id, std::string league, std::string season, std::string hometeam, std::string awayteam, std::string match_start_at);
    void get_matches_without_startdate(std::string league, std::string season);
    void get_matches();
    void update_standing(std::string points, std::string league, std::string season, std::string team, std::string won, std::string draw, std::string lost);
    void update_goalscore(std::string league, std::string season, std::string team, std::string venue, std::string goal, std::string hometeam, std::string awayteam);
    void start_match(std::string league, std::string season, std::string hometeam, std::string awayteam);
    void end_match(std::string league, std::string season, std::string hometeam, std::string awayteam);
};

#endif /* Database_hpp */
