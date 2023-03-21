#pragma once


#include <string>
#include <iostream>
#include <map>

using std::string;
using std::map;

class Game {

private:
    map<string, string> general; // map for general game updates
    map<string, string> updatesA; // map for team a updates
    map<string, string> updatesB; // map for team b updates
    string updates; // string of all the reports of the user
    string teamA; // name of team a
    string teamB; // name of team b
    
public:
    Game(map<string, string> generalUpdates, map<string, string> updatesForA, map<string, string> updatesForB, string teamA_, string teamB_);
    void setGeneral (map<string, string> generalUpdates);
    void setAUpdates (map<string, string> updatesForA);
    void setBUpdates (map<string, string> updatesForB);
    void addUpdates (string overAllUpdates);
    string buildSummary();
};