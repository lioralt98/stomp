package bgu.spl.net.impl.echo;

import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionsImpl;
import bgu.spl.net.srv.Server;

public class EchoServer {

    public static void main(String[] args) {
        Connections<String> connections = new ConnectionsImpl<>();

        // you can use any server... 
        Server.threadPerClient(
                631, //portmakw
                () -> new EchoProtocol(connections), //protocol factory
                LineMessageEncoderDecoder::new //message encoder decoder factory
        ).serve();

        // Server.reactor(
        //         Runtime.getRuntime().availableProcessors(),
        //         7777, //port
        //         () -> new EchoProtocol<>(), //protocol factory
        //         LineMessageEncoderDecoder::new //message encoder decoder factory
        // ).serve();
    }
}
