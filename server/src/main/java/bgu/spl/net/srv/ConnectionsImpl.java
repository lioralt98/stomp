package bgu.spl.net.srv;

import java.util.Deque;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedDeque;
import java.util.concurrent.atomic.AtomicInteger;

public class ConnectionsImpl<T> implements Connections<T> {
    private final Map<String, Deque<String>> topicToSubIdConId;  // topic - subId, connectionId map
    private final Map<Integer, ConnectionHandler<T>> handlers;
    private final AtomicInteger nextUniqueId;

    public ConnectionsImpl() {
        handlers = new ConcurrentHashMap<>();
        nextUniqueId = new AtomicInteger(0);
        topicToSubIdConId = new ConcurrentHashMap<>();

    }

    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> currentCH = handlers.get(connectionId);
        if (currentCH != null) {
            currentCH.send(msg);
            return true;
        }
        return false;
    }

    public void disconnect(int connectionId) {
        handlers.remove(connectionId);
    }

    public void addToTopic(String topic, String subId, int conId) { 
        topicToSubIdConId.computeIfAbsent(topic, k -> new ConcurrentLinkedDeque<>()).add(String.format(subId + ":%d", conId));

    }

    public void removeFromTopic(String topic, String subId, int conId) { 
        topicToSubIdConId.computeIfPresent(topic, (k, v) -> {
            v.remove(String.format(subId + ":%d", conId));
            return v;
        });

    }


    public int connectAndReturnConId(ConnectionHandler<T> connectionHandler) { 
        int conId;
        do {
            conId = nextUniqueId.get();
        } while (!nextUniqueId.compareAndSet(conId, conId + 1));
        handlers.put(conId, connectionHandler);
        return conId;
    }

    public Deque<String> getSubIdsByTopic(String topic) {
        return topicToSubIdConId.get(topic);
    }

}
