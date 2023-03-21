#include "../include/Game.h"

#include <sstream>
#include <string>
#include <iostream>
#include <map>

using std::string;
using std::map;
using std::endl;
using std::stringstream;

Game::Game(map<string, string> generalUpdates, map<string, string> updatesForA, map<string, string> updatesForB, string teamA_, string teamB_) : general(generalUpdates), updatesA(updatesForA), updatesB(updatesForB), updates("") , teamA(teamA_), teamB(teamB_) {}

void Game::setGeneral (map<string, string> generalUpdates) {
    //for every entry in generalUpdates if the entry is not in general, add it and its value
    //if the entry is already in general overwrite the value of it
    for(map<string, string>::const_iterator it = generalUpdates.begin(); it != generalUpdates.end(); it++) {
        general[it->first] = it->second; 
    }
}


void Game::setAUpdates (map<string, string> updatesForA) {
    //for every entry in updatesForA if the entry is not in updatesA, add it and its value
    //if the entry is already in updatesA overwrite the value of it
    for(map<string, string>::const_iterator it = updatesForA.begin(); it != updatesForA.end(); it++) {
        updatesA[it->first] = it->second; 
    }
}

void Game::setBUpdates (map<string, string> updatesForB) {
        //for every entry in updatesForB if the entry is not in updatesB, add it and its value
    //if the entry is already in updatesB overwrite the value of it
    for(map<string, string>::const_iterator it = updatesForB.begin(); it != updatesForB.end(); it++) {
        updatesB[it->first] = it->second; 
    }
}

void Game::addUpdates (string overAllUpdates) {
    //add a new report to the game that was made by the user
    if (updates != "")
        updates = updates + overAllUpdates + "\n\n";
    else
        updates = overAllUpdates + "\n\n";
}

string Game::buildSummary() {
    // compose the summary of the user to a specific topic from all the data that was
    //saved in the fields of this Game object
    stringstream summary;
    summary << teamA << " vs " << teamB << endl;
    summary << "Game stats:" << endl;
    summary << "General stats:" << endl;
    for(map<string, string>::const_iterator it = general.begin(); it != general.end(); it++) {
        summary << it->first << ": " << it->second << endl;
    }
    summary << "\n" ;
    summary << teamA << " stats:" << endl;
    for(map<string, string>::const_iterator it = updatesA.begin(); it != updatesA.end(); it++) {
        summary << it->first << ": " << it->second << endl;
    }
    summary << "\n" ;
    summary << teamB << " stats:" << endl;
    for(map<string, string>::const_iterator it = updatesB.begin(); it != updatesB.end(); it++) {
        summary << it->first << ": " << it->second << endl;
    }
    summary << "\n" ;
    summary << "Game event reports:" << endl;
    summary << updates;

    string final = summary.str();
    return final;
}