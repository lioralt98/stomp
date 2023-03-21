
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "../include/StompProtocol.h"

using namespace std;

vector<string> loginHandler(vector<string>);
string createConnect (vector<string>);

mutex _mutex;
condition_variable condition;
bool alerted = false;
bool volatile connected = false;

void receiverFunc (ConnectionHandler *connectionHandler, StompProtocol &protocol) {
	while(1) { // run forever
		string answer;
    	if (!connectionHandler->getLine(answer)) { // read message from the server, if there are no messages wait for a message to come
        	break;
        }
		int len = answer.length();
		answer.resize(len-1); // delete the last character because its the null character
		string response = protocol.analyzeAnswer(answer); // process the frame that was received from the server
		alerted = true; // set alerted to true to let the thread that reads from the keyboard that we got the response from the server
		if (response == "CONNECTED") { // if the response is a CONNECTED frame we need to notify the thread that reads from the keyboard
			unique_lock<mutex> lock(_mutex);
			condition.notify_one();
		}
		if (response == "DISCONNECT" || response == "ERROR") { // if the response is a DISCONNECT or ERROR frame
															//we need to close the socket of the user with the server
			connectionHandler->close();// close the socket
			connected = false; // set connected to false to only read login commands from the keybaord
			protocol.setConnected(false);
			delete connectionHandler; // delete the connectionHandler object because he's allocated on the heap
										//to avoid memory leaks
			connectionHandler = nullptr; // set the pointer to nullptr to avoid memory leaks
			cout << "disconnected" << endl; // print to the screen disconnected and notify the other thread that is maybe sleeping
			{
				unique_lock<mutex> lock(_mutex);
				condition.notify_one();
			}
			break; //end the loop to kill this thread, he will be created again when user is logged in
		}
    	
	}
}

/**
* This code assumes that the server replies the exact text the client sent it (as opposed to the practical session example)
*/
int main (int argc, char *argv[]) {
	const short bufsize = 1024;
    char buf[bufsize];

	string input;
	StompProtocol protocol;
	ConnectionHandler *connectionHandler;

	while (1) {  // run forever
        std::cin.getline(buf, bufsize); // read input from the keyboard by the user
		std::string input(buf);
		vector<string> parts; // break the input of the user to parts of data to process it later on
		boost::split(parts,input,boost::is_any_of(" "), boost::token_compress_on); //divides the message to parts by the " "(spacing) and puts in parts
		string command = parts[0]; //taking out the next command

		if (!connected) { // if false no one is logged in so only a login command will be dealt with
							//other commands are ignored
			if (command == "login") { 
				vector<string> login_info = loginHandler(parts); // get a vector of login info like host and port
				connectionHandler = new ConnectionHandler(login_info.at(0), stoi(login_info.at(1))); // create a new connection handler on HEAP!!
				if (!connectionHandler->connect()) { // connect to the server
        			std::cerr << "Could not connect to server" << std::endl;
        			return 1;
				}
				connected = true;
				protocol.setConnected(true);
				protocol.setUsername(login_info.at(2)); // save the username of the user for later use
				string frame = createConnect(login_info); //create CONNECT frame 
				if (!connectionHandler->sendLine(frame)) { // send the CONNECT frame
					connectionHandler->close();
					connected = false;
					protocol.setConnected(false);
				}
				thread receiver(receiverFunc, connectionHandler, std::ref(protocol)); // create a thread to read messages from the socket that the server is sending
				receiver.detach(); //start the thread
				{ // wait until the other thead gets CONNECTED frame from the server to confirm login 
					unique_lock<mutex> lock(_mutex);
					while(!alerted)
						condition.wait(lock);
				}
				alerted=false; // reset the flag
				 									//if the connectionHandler did send
        	}
			continue;							
		}
		string frame = protocol.process(parts); // process the input of the user
		if (frame == "") continue; // if nothing is needed to be sent to the server jump to start of the while loop to read new input from user
		if (!connectionHandler->sendLine(frame)) {
		}
		if (command == "logout") { // only if the command of the user is logout wait for the other thread
									//to get a RECEIPT to confirm the logout of the user
			unique_lock<mutex> lock(_mutex);
			while(!alerted)
				condition.wait(lock);
		}
		alerted = false;


	}
	delete connectionHandler; // will not get to this part (no break in the while), just for insurance
	connectionHandler = nullptr;
	return 0;
}

vector<string> loginHandler(vector<string> parts) {
	//get all the info from the input of the user to login to the server
	string host_port = parts[1];
	int delimeter = parts[1].find(':');
	string host = parts[1].substr(0, delimeter);
	string port = parts[1].substr(delimeter + 1);
	string username = parts[2];
	string passcode = parts[3];
	vector<string> result{host, port, username, passcode};
	return result;
}

string createConnect (vector<string> parts) {
	//create a CONNECT frame
	stringstream frame;
	frame << "CONNECT" << endl;
	frame << "accept-version:1.2" << endl;
	frame << "host:stomp.cs.bgu.ac.il" << endl;
	frame << "login:" +parts.at(2) << endl;
	frame << "passcode:" + parts.at(3) +"\n\n" << endl;
	string result = frame.str();
	return result;
}
