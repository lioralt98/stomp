package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionsImpl;
import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        Connections<String> connections = new ConnectionsImpl<>();

        int port = Integer.parseInt(args[0]); // get the port from the args
        if (args[1] == "tpc") { // create a TPC server
             Server.threadPerClient(
                     port, //port
                     () -> new StompMessagingProtocolImpl(connections), //protocol factory
                     NullMessageEncoderDecoder::new //message encoder decoder factory
             ).serve();
         }
         else if (args[1] == "reactor") { // create a REACTOR server
             Server.reactor(3, 
                     port, 
                     () -> new StompMessagingProtocolImpl(connections),
                     NullMessageEncoderDecoder::new
             ).serve();
        }
    }
}