package bgu.spl.net.impl.stomp;


import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.ConnectionHandler;
import bgu.spl.net.srv.Connections;


import java.util.Deque;
import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<String> {

    Integer myConId;

//    private final static Map<Integer, Integer> conIdToSubId = new ConcurrentHashMap<>(); // connectionId - subscriptionId map

    private final static Map<String, String> usersData = new ConcurrentHashMap<>(); // username - password map
    // all connectionHandlers, thread safe

    private final static Map<String, Boolean> connectedUsers = new ConcurrentHashMap<>(); // username - boolean connected map

    private final static Map<Integer, String> conIdToUsername = new ConcurrentHashMap<>(); // connectionId - username map

    private final Map<String, String> subIdToTopic; // subId - Topic map local (for unsub purpose)

    private final Connections<String> connections; // object that manages sending messages to clients that are connected to the server

    private boolean shouldTerminate; // true if the server should end the connection with the client, like for an error

    private final static AtomicInteger nextMessageId = new AtomicInteger(0); // increase messageId atomicly to make it thread safe

    public StompMessagingProtocolImpl(Connections<String> connections) {
        shouldTerminate = false;
        this.connections = connections;
        subIdToTopic = new HashMap<>();
    }

    public Integer getMyConId() { // returns the Connection handler's connection id
        return myConId;
    }

    public void start(ConnectionHandler<String> connectionHandler) { // give the connection handler a unique connection id and register the CH in connections object
        myConId = connections.connectAndReturnConId(connectionHandler);
    }

    public void process(String message) { 
        // proccess the message according to the frame that was recieved from the client
        String[] parts = message.split("\n"); // seperate the frame by new lines
        String command = parts[0]; // get the command of the frame
        int j = 0;
        Map<String, String> headers = new HashMap<>(); // get all the headers of the frame
        for (int i = 1; i < parts.length; i++) {
            if (parts[i].equals("")) {
                j = i; // j is now the empty string before the frame body, if exists
                break;
            }
            String[] header = parts[i].split(":");
            headers.put(header[0], header[1]);
        }
        j = j + 1;
        String body = "";
        for (; j < parts.length; j++) //iterating to the end because we know encode removes the last null, get the body of the frame
            body = body + (parts[j] + "\n");

        switch (command) {
            case "CONNECT":
                handleConnect(headers, message); // if command is CONNECT
                break;
            case "SEND":
                handleSend(headers,message, body); // if command is SEND
                break;
            case "SUBSCRIBE":
                handleSubscribe(headers, message); // if command is SUBSCRIBE
                break;
            case "UNSUBSCRIBE":
                handleUnsubscribe(headers, message); // if command is UNSUBSCRIBE
                break;
            case "DISCONNECT":
                handleDisconnect(headers, message); // if command is DISCONNECT
                break;
            default:
                handleIllegalCommandName(headers, message); // if command is not recognized, is illegal
                break;
        }
    }

    private void handleSend(Map<String, String> headers, String frame, String body) {
        String response;
        String topic = headers.get("destination"); // get the topic from the frame, by destination header
        if (topic == null) { // if topic is missing from the frame, return an error frame.
            response = createErrorFrame("malformed frame received",
                    "Did not contain a destination header, which is REQUIRED to send propagation",
                    "",
                    frame);
            connections.send(myConId, response); // send the error frame
            disconnect(); // close the connection with the client due to an error
        }
        else {
            Deque<String> relevantIds = connections.getSubIdsByTopic(topic); // get all the subscription ids of all the users in the topic we want to send a message to

            for (String subIdConId : relevantIds) { // from every user subscribed to this topic, he take the connection id and the subscription id
                String[] ids = subIdConId.split(":");
                String subId = ids[0];
                int conId = Integer.parseInt(ids[1]);
                response = createMessageFrame(topic, subId, body); // create a message frame with the subscription id of the user
                connections.send(conId, response); // send the message frame to the user with his connection id
            }
        }
    }

    private void handleSubscribe(Map<String, String> headers, String frame) {
        String response;
        String topic = headers.get("destination"); // get the topic from the frame
        String subId = headers.get("id"); // get the subscription id from the frame
        String receiptId = headers.get("receipt"); // get the receipt id from the frame
        if (receiptId == null) {//meaning destination was spelled wrong or wasn't found
            response = createErrorFrame("malformed frame received",
                    "Did not contain a receipt header, which is REQUIRED to subscribe",
                    "",
                    frame);
            connections.send(myConId, response);
            disconnect();

        } else if (conIdToUsername.get(myConId) == null) { //client's first frame isn't a connect frame
            response = createErrorFrame("client did not log in",
                    "Did not receive a CONNECT frame from this client",
                    receiptId,
                    frame);
            connections.send(myConId, response);
            disconnect();

        } else if (subId == null) { // client's subscription id is missing in the frame
            response = createErrorFrame("malformed frame received",
                    "Did not contain an id header, which is REQUIRED to subscribe",
                    receiptId,
                    frame);
            connections.send(myConId, response);
            disconnect();

        } else if (topic == null) { // client's topic is missing in the frame
            response = createErrorFrame("malformed frame received",
                    "Did not contain a destination header, which is REQUIRED to subscribe",
                    receiptId,
                    frame);
            connections.send(myConId, response);
            disconnect();

        } else {
            connections.addToTopic(topic, subId, myConId); //create a topic and add the corresponding id / add the id to an existing topic
            subIdToTopic.put(subId, topic); //linking the sub id to the topic it subscribed to
            response = createReceiptFrame(receiptId);
            connections.send(myConId, response);
        }
    }

    private void handleUnsubscribe(Map<String, String> headers, String frame) {
        String response;
        String subId = headers.get("id");
        String receiptId = headers.get("receipt");
        if (receiptId == null) {//no receipt was in the frame
            response = createErrorFrame("malformed frame received",
                    "Did not contain a receipt header, which is REQUIRED to unsubscribe",
                    "",
                    frame);
            connections.send(myConId, response);
            disconnect();

        } else if (conIdToUsername.get(myConId) == null) { //client's first frame isn't a connect frame
            response = createErrorFrame("client did not log in",
                    "Did not receive a CONNECT frame from this client",
                    receiptId,
                    frame);
            connections.send(myConId, response);
            disconnect();

        } else if (subId == null) { // client's subscription id is missing from the frame
            response = createErrorFrame("malformed frame received",
                    "Did not contain an id header, which is REQUIRED to unsubscribe",
                    receiptId,
                    frame);
            connections.send(myConId, response);
            disconnect();

        } else if (subIdToTopic.get(subId) == null) { //no topic is linked to the requested subId
            response = createErrorFrame("subscription-id is not found",
                    "No topic was found that is associated with the requested subscription-id in order to UNSUBSCRIBE",
                    receiptId,
                    frame);
            connections.send(myConId, response);
            disconnect();

        } else { //all the conditions meet and we can remove the subscription-id
            String topic = subIdToTopic.remove(subId);
            connections.removeFromTopic(topic, subId, myConId); // remove his subscription id
            response = createReceiptFrame(receiptId);
            connections.send(myConId, response);
        }
    }

    private void handleDisconnect(Map<String, String> headers, String frame) {
        String response;
        String receiptId = headers.get("receipt");
        if (receiptId == null) {//no receipt was in the frame
            response = createErrorFrame("malformed frame received",
                    "Did not contain a receipt header, which is REQUIRED to disconnect",
                    "",
                    frame);
        }
        else if (conIdToUsername.get(myConId) == null) { //client's first frame isn't a connect frame
            response = createErrorFrame("client did not log in",
                    "Did not receive a CONNECT frame from this client",
                    receiptId,
                    frame);
        }
        else // all conditions are met and we can reply with a receipt frame
            response = createReceiptFrame(receiptId);

        connections.send(myConId, response);
        for (Map.Entry<String, String> entry : subIdToTopic.entrySet()) { // delete all the subs of the user from all his topics
            String subId = entry.getKey();
            String topic = entry.getValue();
            connections.removeFromTopic(topic, subId, myConId);
        }
        connections.disconnect(myConId);
        String username = conIdToUsername.remove(myConId); // get username by his connection id
        if (username != null) // delete the user from the connected users list
            connectedUsers.remove(username);
    }


    private void handleConnect(Map<String, String> headers, String frame) {
        String response;
        String version = headers.get("accept-version");
        String username = headers.get("login");
        String passcode = headers.get("passcode");

        if (usersData.get(username) == null) {                   //case 1 - new user is created
            usersData.put(username, passcode);
            connectedUsers.put(username, true);
            conIdToUsername.put(myConId, username);
            response = createConnectedFrame(version);
            connections.send(myConId, response);

        } else if (connectedUsers.get(username) != null) {      // case 2 - user is already logged in - check if
            response = createErrorFrame("user already logged in",
                    "User is already logged in to the server, please logout before logging in again.",
                    "",
                    frame);
            connections.send(myConId, response);
            disconnect();

        } else if (!usersData.get(username).equals(passcode)) { // case 3 - password is incorrect
            response = createErrorFrame("wrong passcode",
                    "Wrong passcode, please try again.",
                    "",
                    frame);
            connections.send(myConId, response);
            disconnect();

        } else {                                                // case 4 - default case, everything is OK
            connectedUsers.put(username, true); // register user as connected in the connected users list
            conIdToUsername.put(myConId, username); // link the connection id of the user to his username
            response = createConnectedFrame(version);
            connections.send(myConId, response);

        }
    }

    private void handleIllegalCommandName(Map<String, String> headers, String frame) {
        String response;
        String receiptId = headers.get("receipt");
        if (receiptId == null) {//no receipt was in the frame
            response = createErrorFrame("illegal command",
                    "Provided command is not recognized.",
                    "",
                    frame);
        }
        else { // send the same error but without a receipt id, because it's missing
            response = createErrorFrame("illegal command",
                    "Provided command is not recognized.",
                    receiptId,
                    frame);
        }

        connections.send(myConId, response);
        disconnect();
    }

    private String createConnectedFrame(String version) { // create a CONNECTED frame
        return String.format("CONNECTED\nversion:%s\n", version);
    }

    private String createErrorFrame(String message_header, String message, String receipt_header, String frame) {
        // create an ERROR frame
        if (receipt_header.equals(""))
            return String.format("ERROR\nmessage:%s\n\nThe message:\n-----\n%s\n-----\n%s"
                , message_header, frame, message);
        return String.format("ERROR\nreceipt:%s\nmessage: %s\n\nThe message:\n-----\n%s\n-----\n%s"
                ,receipt_header, message_header, frame, message);
    }

    private String createReceiptFrame(String receiptId) { //create a RECEIPT frame
        return String.format("RECEIPT\nreceipt-id:%s\n", receiptId);
    }

    private String createMessageFrame(String topic, String subId, String body) { // create a MESSAGE frame
        int msgId;
        do {
            msgId = nextMessageId.get();
        } while (!nextMessageId.compareAndSet(msgId, msgId + 1));
        return String.format("MESSAGE\nsubscription:%s\nmessage-id:%d\ndestination:%s\n\n%s", subId, msgId, topic, body);
    }

    private void disconnect() { // when we disconnect from a user (in an error) from our side, remove all the subs of the user from all his topics
                                // and remove him from the connected users list and force the socket to close with shouldTerminate set to true
        shouldTerminate = true;
        for (Map.Entry<String, String> entry : subIdToTopic.entrySet()) {
            String subId = entry.getKey();
            String topic = entry.getValue();
            connections.removeFromTopic(topic, subId, myConId);
        }
        connections.disconnect(myConId);
        String username = conIdToUsername.remove(myConId);
        if (username != null)
            connectedUsers.remove(username);
    }


    /**
     * @return true if the connection should be terminated
     */
    public boolean shouldTerminate() {
        return shouldTerminate;
    }
}
