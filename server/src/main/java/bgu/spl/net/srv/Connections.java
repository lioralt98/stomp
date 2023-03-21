package bgu.spl.net.srv;

import java.util.Deque;

public interface Connections<T> {

    boolean send(int connectionId, T msg);

    void disconnect(int connectionId);

    void addToTopic(String topic, String subId, int conId);

    void removeFromTopic(String topic, String subId, int conId);

    int connectAndReturnConId(ConnectionHandler<T> connectionHandler);

    Deque<String> getSubIdsByTopic(String topic);
}
