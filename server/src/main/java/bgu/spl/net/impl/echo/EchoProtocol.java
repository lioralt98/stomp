package bgu.spl.net.impl.echo;

import bgu.spl.net.api.MessagingProtocol;
import bgu.spl.net.srv.ConnectionHandler;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionsImpl;
import bgu.spl.net.api.StompMessagingProtocol;

import java.time.LocalDateTime;

public class EchoProtocol implements StompMessagingProtocol<String> {


    Connections<String> connections;
    private boolean shouldTerminate = false;

    private int conId;

    public EchoProtocol(Connections<String> connections) {
        this.connections = connections;
        conId = -1;
    }

    @Override
    public void process(String msg) {
        shouldTerminate = "bye".equals(msg);
        System.out.println("[" + LocalDateTime.now() + "]: " + msg);
//        String res = createEcho(msg);
        connections.send(conId, msg);
    }

    private String createEcho(String message) {
        String echoPart = message.substring(Math.max(message.length() - 2, 0), message.length());
        return message + " ,, " + echoPart + " .. " + echoPart + " ..";
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    public void start(ConnectionHandler<String> connectionHandler) {
        conId = connections.connectAndReturnConId(connectionHandler);
    }

    public Integer getMyConId() {
        return conId;
    }
}
