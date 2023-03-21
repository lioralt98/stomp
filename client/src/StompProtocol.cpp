#include <sstream>
#include <fstream>
#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include "../include/StompProtocol.h"
#include "../include/event.h"
// #include "../include/Game.h"

using std::string;
using std::vector;
using std::stringstream;
using std::ofstream;
using std::pair;
using std::transform;
using std::cout;
using std::endl;
using std::map;


StompProtocol::StompProtocol() : isConnected(false), username(""), nextSubId(0), nextReceipt(0), topicToSubId(), receiptToCommand(), topicToUserAndGame(){}
 

string StompProtocol::process (vector<string> parts) {
   // process the command of the user and send a frame to the server according to his command
   string response = "";
   string command = parts[0];
   if (command == "login")
      handleLogin(); // if his command is login
   else if (command == "join") 
      response = handleJoin(parts); // if his command is join
   else if (command == "report")
      response = handleReport(parts); // if his command is report
   else if (command == "exit")
      response = handleExit(parts); // if his command is exit
   else if (command == "logout")
      response = handleLogout(parts); // if his command is logout
   else if (command == "summary")
      response = handleSummary(parts); // if his command is summary
   else // any other command is not recognized, send back an empty string to do nothing
      response = "";
   
   return response;

}

string StompProtocol::analyzeAnswer(string answer) {
   // process the frame that the client received from the server
   //first break the frame to smaller parts of data
   vector<string> parts;
   string command;
   map<string, string> headers;
   string holder;
   string body;

   stringstream frame(answer); // create a string stream to read the frame
   getline(frame, command); // the the first line of the frame and save it in the command variable
   string currHeader;
   while(getline(frame,currHeader) && currHeader != "") {
      //until we hit an empty line, these are the frame's headers, save them in a map
      //where the header name is the key and the header value is the value
      string key, value;
      stringstream header(currHeader);
      getline(header,key, ':'); // save in key variable the string before the ":", this is the header name
      getline(header,value); // save the rest of the line in the value variable, this is the header value
      headers[key] = value; // save in the map
   }
   while (getline(frame, holder)) // after we hit an empty line, every line from now on is part of the body
      body += holder + "\n"; // get each line and add to the body variable

   string response;
   // handle the frame according the the frame's command
   if (command == "CONNECTED")
      response = handleConnected(); // if his command is CONNECTED
   else if (command == "RECEIPT")
      response = handleReceipt(stoi(headers["receipt-id"])); // if his command is RECEIPT
   else if (command == "MESSAGE")
      response = handleMessage(body); // if his command is MESSAGE
   else
      response = handleError(headers["message"]); // if his command is ERROR

   return response;

}

string StompProtocol::handleConnected() {
   // when we get a CONNECTED frame from the server, simply print this to the screen of the user
   cout << "Login successful" << endl;
   return "CONNECTED"; // return CONNECTED for later use
}

string StompProtocol::handleReceipt(int receiptId) {
   //get the command that the user made in his terminal by the receipt id that was generated for this command
   //only when the receipt id is linked to a logout command do something, clear all the subs of the user
   //that he made to topics and clear the map of the receipt id to commands of the user
   if (receiptToCommand[receiptId] == "logout") {
      topicToSubId.clear();
      receiptToCommand.clear();
      return "DISCONNECT"; // return DISCONNECT for later use
   }
   return "RECEIPT"; // return RECEIPT for later use
}

string StompProtocol::handleMessage(string body) {
   //when we get a MESSAGE frame we need to update the contents (body) in the Game
   //object of the user that sent the message in the topic that he messaged to
   //first break the body of the frame to smaller parts of data
   string username, team_a, team_b, event_name, time, description;
   map<string, string> general_game_updates, team_a_updates, team_b_updates;
   stringstream frame(body);
   string line;

   while(getline(frame, line) && line.find("general game updates") == line.npos) {
      //everything until he hit "general game updates" is an header
      //take the contents of the headers to variables
      if (line.find("user") != line.npos) {
         //get the username of the user that sent the message to the topic
         int index = line.find(":");
         username = line.substr(index+2);
      }
      else if (line.find("team a") != line.npos) {
         //get the team a's name and save to team_a variable
         int index = line.find(":");
         team_a = line.substr(index+2);
      }
      else if (line.find("team b") != line.npos) {
         //get the team b's name and save to team_b variable
         int index = line.find(":");
         team_b = line.substr(index+2);
      }
      else if (line.find("event name") != line.npos) {
         //get the event name and save to event_name variable
         int index = line.find(":");
         event_name = line.substr(index+2);
      }
      else if (line.find("time") != line.npos) {
         //get the time and save to time variable
         int index = line.find(":");
         time = line.substr(index+2);
      }
   }

   while(getline(frame, line) && line.find("team a updates") == line.npos) {
      //until we hit the line "team a updates" every line is a "general game" stat
      unsigned index = line.find(":");
      //for every line find the ":" and save the stat name before the ":" and the stat value after the ":"
      //in a map for "general game updates"
      if (index != line.npos) {
         string key = line.substr(0, index);
         string value = line.substr(index+2);
         general_game_updates[key] = value;
      }
   }

   while(getline(frame, line) && line.find("team b updates") == line.npos) {
      //until we hit the line "team b updates" every line is a "team a" stat
      unsigned index = line.find(":");
      //for every line find the ":" and save the stat name before the ":" and the stat value after the ":"
      //in a map for "team a updates"
      if (index != line.npos) {
         string key = line.substr(0, index);
         string value = line.substr(index+2);
         team_a_updates[key] = value;
      }
   }

   while(getline(frame, line) && line.find("description") == line.npos) {
      //until we hit the line "description" every line is a "team b" stat
      unsigned index = line.find(":");
      //for every line find the ":" and save the stat name before the ":" and the stat value after the ":"
      //in a map for "team b updates"
      if (index != line.npos) {
         string key = line.substr(0, index);
         string value = line.substr(index+2);
         team_b_updates[key] = value;
      }
   }

   while(getline(frame, line)) // every line from now on is part of the description, save all the lines in the description variable
      description += line + "\n";
   

   //the next part's purpose is to handle the game object of the reporting user 
   
   string topic = team_a + "_" + team_b; // compose the topic name

   stringstream gameEvent; // create a string stream to write the report of the user in order to save it in the Game object

   //write the report 
   gameEvent << time << " - " << event_name << ":\n" << endl;
   gameEvent << description;

   string sum_event = gameEvent.str(); // turn to a string 
   if (topicToUserAndGame.count(topic) == 0) { //if the topic doesn't exist yet, create a new entry for the topic and add the update
      map<string, Game> userToGame; 
      Game game(general_game_updates, team_a_updates, team_b_updates, team_a, team_b); //create the relevant game for the topic for the specific user and update the fields
      game.addUpdates(sum_event);
      userToGame.insert(pair<string, Game>(username, game)); //initialize the username's game
      topicToUserAndGame[topic] = userToGame; ////initialize the topic
   }
   else if (topicToUserAndGame[topic].count(username) == 0) { //user didn't update the topic yet at all but topic exists
      map<string, Game> &userToGame = topicToUserAndGame[topic];
      Game game(general_game_updates, team_a_updates, team_b_updates, team_a, team_b); //create the game, update the fields
      game.addUpdates(sum_event); //update the game reports
      userToGame.insert(pair<string, Game>(username, game)); //initialize the username's game
   }
   else { //topic and specific user already exist
      map<string, Game> &userToGame = topicToUserAndGame[topic]; 
      Game &game = userToGame.at(username);
      //updating all the fields from here
      game.addUpdates(sum_event); 
      game.setGeneral(general_game_updates);
      game.setAUpdates(team_a_updates);
      game.setBUpdates(team_b_updates);
   }
   return "MESSAGE";
}

string StompProtocol::handleError(string messageHeader) {
   cout << messageHeader << endl; // print to the screen the error's header
   // clear the subs that the user made to topics and the receipt to commands map because the connection terminated by the server
   //because he sent an error frame
   topicToSubId.clear();
   receiptToCommand.clear();
   return "ERROR";
}



void StompProtocol::handleLogin() {
   // if isConnected is true, a login already occured so print to the screen that someone is already logged in
   if (isConnected) {
      cout << "The client is already logged in, log out before trying again" << endl;
      isConnected = true;
   }
 }

string StompProtocol::handleJoin(vector<string> parts) {
   // when we get a join command, join the topic that the user sent
   string topic = parts[1];
   int subId = nextSubId++; // create a new subscription id
   int receiptId = nextReceipt++; //create a new receipt id
   topicToSubId[topic] = subId; // save the subscription id and link it to the topic
   receiptToCommand[receiptId] = parts[0]; // save the receipt id and link it to the join command
   
   stringstream frame; // create a SUBSCRIBE frame to send to the server
	frame << "SUBSCRIBE" << endl;
	frame << "destination:" << topic << endl;
	frame << "id:" << subId << endl;
	frame << "receipt:" << receiptId << "\n\n" << endl;
	string result = frame.str();

   cout << "Joined channel " << topic << endl;

	return result;
}

string StompProtocol::handleReport(vector<string> parts) {
   //when we get a report command, we need to send a SEND frame to the server with the report of the user
   //later on the server will send the frame as a MESSAGE frame to all the users in the topic that
   //the report was sent to
   string path = parts[1];
   stringstream frame; // write a SEND frame
   names_and_events nne = parseEventsFile(path); // parse the report of the user
   string topic = nne.team_a_name + "_" + nne.team_b_name; // compose the topic
   vector<Event> events = nne.events; // get all the events that the user wants to report on to the topic
   if (topicToSubId.count(topic) == 0) return ""; // check if the user is subscribed to the topic that he wants to send a report to

   for (auto &event : events) { // for every event create a seperate SEND frame
   //simply fill the SEND frame layout with the data of the report that we got from the parser function
      frame << "SEND" <<endl;
      frame << "destination:" << topic << "\n"<< endl;
      frame << "user: " << username << endl;
      frame << "team a: " << event.get_team_a_name() << endl;
      frame << "team b: " << event.get_team_b_name() << endl;
      frame << "event name: " << event.get_name() << endl;
      frame << "time: " << event.get_time() << endl;
      frame << "general game updates:" << endl;
      //iterate over all the stats and add them to the report
      for(map<string, string>::const_iterator it = event.get_game_updates().begin(); it != event.get_game_updates().end(); it++) {
         frame << "  " << it->first << ": " << it->second << endl;
      }
      frame << "team a updates:" << endl;
      for(map<string, string>::const_iterator it = event.get_team_a_updates().begin(); it != event.get_team_a_updates().end(); it++) {
         frame << "  " << it->first << ": " << it->second << endl;
      }
      frame << "team b updates:" << endl;
      for(map<string, string>::const_iterator it = event.get_team_b_updates().begin(); it != event.get_team_b_updates().end(); it++) {
         frame << "  " << it->first << ": " << it->second << endl;
      }
      frame << "description:" << endl;
      frame << event.get_discription() << "\n" << '\0'; // add a null character at the end of the SEND message
                                                      //to mark the end of the frame
   }

   string result = frame.str(); // turn to string
   int len = result.length();
   result.resize(len-1); // delete the last null character that was added because the encoder will add one anyway
   return result; // return all the SEND frames as a single string to send to the server

}

string StompProtocol::handleExit(vector<string> parts) {
   //when the user sends a exit command, unsubscribe from the topic he sent
   string topic = parts[1];
   if (topicToSubId.count(topic) == 0) return ""; // check if the user is subscribed to the topic, cant unsub if the user is not subbed to it
   int subId = topicToSubId[topic];
   topicToSubId.erase(topic); // delete the subscription id from the map to unlink the topic from the user
   int receiptId = nextReceipt++; // generate a new receipt id
   receiptToCommand[receiptId] = parts[0]; // save the receipt id as the exit command

   stringstream frame; // create the SUBSCRIBE frame to send to the server
	frame << "UNSUBSCRIBE" << endl;
	frame << "id:" << subId << endl;
	frame << "receipt:" << receiptId << "\n" << endl;
	string result = frame.str();

   cout << "Exited channel " << topic << endl;

	return result;
}

string StompProtocol::handleLogout(vector<string> parts) {
   // if the user sent a logout command, delete all the subscription ids of the user to unlink him from all the topics he subbed to
   // and close the connection later on 
   int receiptId = nextReceipt++; // generate a new receipt id
   topicToSubId.clear();
   receiptToCommand[receiptId] = parts[0];

   stringstream frame; // create a DISCONNECT frame
	frame << "DISCONNECT" << endl;
	frame << "receipt:" << receiptId << "\n" << endl;
	string result = frame.str();

	return result;
}

string StompProtocol::handleSummary(vector<string> parts) {
   // when the user sends a summary command, create or overwrite a file with a summary of all the reports
   // the username made on the topic (username and topic provided by the user)
   string topic = parts[1];
   string username = parts[2];
   string path = parts[3];
   if (topicToSubId.count(topic) == 0) //if the user is not subscribed to this topic, he cannot request a summary of it
      return "";
   if (topicToUserAndGame.count(topic) != 0) { //checking if topic is present, if there are updates in it
      map<string, Game> &userToGame = topicToUserAndGame[topic];
      if (userToGame.count(username) != 0) { //checking if the user exists in this topic
         Game &game = userToGame.at(username); // get the game object containing all the data that the requested user made on the requested topic
         ofstream file(path); // create new file or overwrite if exists 
         string sum = game.buildSummary(); // get the summary
         file << sum << endl; //write the summary to the file
      } 
   }
   return "";

}

void StompProtocol::setConnected(bool status) {
   isConnected = status;
}

void StompProtocol::setUsername (string username) {
   this->username = username;
}



 