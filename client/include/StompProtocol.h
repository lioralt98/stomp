#pragma once

#include "../include/ConnectionHandler.h"
#include "../include/Game.h"

class Game;

// TODO: implement the STOMP protocol
class StompProtocol
{
private:
    bool isConnected; // indicate wether a user is connected
    std::string username; // saves the username of the connected user
    int nextSubId; // used to generate unique subscription ids
    int nextReceipt;// used to generate unique receipt ids
    std::map<std::string, int> topicToSubId; // a map from topic to subsription id
    std::map<int, std::string> receiptToCommand; // a map from receipt id to command that was sent with this id
    std::map<std::string, std::map<std::string, Game>> topicToUserAndGame; // map from topic to a map of users made reports to this topic and a Game object that 
                                                                            //contains all the data that was reported by the user
    // std::map<std::string, Game> topicToGame;
public:
    StompProtocol();
    std::string process (std::vector<std::string> parts);
    std::string analyzeAnswer (std::string answer);
    void handleLogin();
    void setConnected(bool status);
    std::string handleJoin(std::vector<std::string> parts);
    std::string handleReport(std::vector<std::string> parts);
    std::string handleExit(std::vector<std::string> parts);
    std::string handleLogout(std::vector<std::string> parts);
    std::string handleSummary(std::vector<std::string> parts);
    std::string handleConnected();
    std::string handleReceipt(int receiptId);
    std::string handleMessage(std::string body);
    std::string handleError(std::string frame);
    void setUsername (std::string username);
};
