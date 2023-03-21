package bgu.spl.net.api;

import bgu.spl.net.srv.ConnectionHandler;

public interface StompMessagingProtocol<T> {
	/**
	 * Used to initiate the current client protocol with it's personal connection ID and the connections implementation
	**/
    void start(ConnectionHandler<T> connectionHandler);

    void process(T message);

    Integer getMyConId();
	
	/**
     * @return true if the connection should be terminated
     */
    boolean shouldTerminate();
}
