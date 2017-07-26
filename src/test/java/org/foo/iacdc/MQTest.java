package org.foo.iacdc;

import java.util.concurrent.ExecutorService;

import junit.framework.TestCase;

import com.rabbitmq.client.Address;

public class MQTest extends TestCase {
    
    public void testDoIt() throws Exception {
        final String user = System.getProperty("iampp.user");
        final String password = System.getProperty("iampp.password");
        final Address[] addrs = new Address[] { new Address(System.getProperty("iampp.host")) };
        final ExecutorService sharedExecutor = null;
        final int nThreads = 5;
        final String exchangeName = EXCHANGE_NAME;
        final String msgNodeId = null;
        final String clientKind = CLIENT_KIND;
        
        new MQListener().start(addrs, user, password, exchangeName, clientKind, msgNodeId, sharedExecutor,
                nThreads);
    }
    
    private final static String EXCHANGE_NAME = "iacdc-dev-exchange";
    
    public static final String CLIENT_KIND = "MQTest";
}
