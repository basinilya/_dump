package org.foo.iacdc;

import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.TimeZone;
import java.util.UUID;
import java.util.prefs.Preferences;

import junit.framework.TestCase;

import org.apache.commons.lang.RandomStringUtils;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.rabbitmq.client.AMQP;
import com.rabbitmq.client.AMQP.BasicProperties;
import com.rabbitmq.client.BuiltinExchangeType;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.Consumer;
import com.rabbitmq.client.DefaultConsumer;
import com.rabbitmq.client.Envelope;
import com.rabbitmq.client.MessageProperties;
import com.spr.ajwf.methods.jobs.DummyForPrefs;

public class MQTest extends TestCase {
    
    public static final String KEY_MSG_NODE_ID = "msgNodeId";
    
    public static final String CLIENT_KIND = "MQTest";
    
    public void testDoIt() throws Exception {
        
        final String exchangeName = EXCHANGE_NAME;
        String msgNodeId = null;
        
        boolean isExclConsume = true;
        
        if (msgNodeId != null) {
            isExclConsume = false; // cluster
        }
        
        final String clientKind = CLIENT_KIND;
        //
        //
        final ConnectionFactory factory = new ConnectionFactory();
        factory.setHost(System.getProperty("iampp.host"));
        final String user = System.getProperty("iampp.user");
        if (user != null) {
            factory.setUsername(user);
        }
        final String password = System.getProperty("iampp.password");
        if (password != null) {
            factory.setPassword(password);
        }
        final Connection connection = factory.newConnection();
        
        final Preferences userPrefs = Preferences.userNodeForPackage(DummyForPrefs.class);
        msgNodeId = userPrefs.get(KEY_MSG_NODE_ID, null);
        
        String bindingKey = null;
        
        for (boolean queueIsNew = false;;) {
            final Channel responseChannel = connection.createChannel();
            
            if (msgNodeId == null) {
                queueIsNew = true;
                msgNodeId = RandomStringUtils.randomAlphanumeric(8);
            }
            bindingKey = AMPP_OUT + "." + msgNodeId + "." + clientKind;
            final String queueName = bindingKey;
            
            responseChannel.queueDeclare(queueName, true, false, false, null);
            
            responseChannel.exchangeDeclare(exchangeName, BuiltinExchangeType.TOPIC);
            responseChannel.queueBind(queueName, exchangeName, bindingKey);
            
            final Consumer responseConsumer = new DefaultConsumer(responseChannel) {
                
                @Override
                public void handleDelivery(final String consumerTag, final Envelope envelope,
                        final AMQP.BasicProperties properties, final byte[] body)
                        throws IOException {
                    final String message = new String(body, "UTF-8");
                    System.out.println(" [x] Received '" + message + "'");
                    try {
                        doWork(message);
                    } catch (final InterruptedException e) {
                        System.out.println(" [x] Interrupted");
                    } finally {
                        System.out.println(" [x] Done");
                        responseChannel.basicAck(envelope.getDeliveryTag(), false);
                    }
                }
            };
            
            final boolean isAutoAck = false;
            try {
                responseChannel.basicConsume(queueName, isAutoAck, "", true, isExclConsume, null,
                        responseConsumer);
                if (queueIsNew) {
                    userPrefs.put(KEY_MSG_NODE_ID, msgNodeId);
                }
                break;
            } catch (final IOException e) {
                // channel was implicitly closed
                if (queueIsNew) {
                    disposeQueue(connection, queueName);
                }
                if (queueIsNew || !isExclConsume) {
                    throw e;
                }
                
                msgNodeId = null;
            }
        }
        
        if ("".length() == 0) {
            return;
        }
        //
        //
        
        // userPrefs.put(MSG_NODE_ID, msgNodeId);
        
        final ObjectMapper mapper = new ObjectMapper();
        final SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ssXXX");
        df.setTimeZone(TimeZone.getTimeZone("UTC"));
        mapper.setDateFormat(df);
        
        AMPPJsonRequest req = null;
        
        req = new AMPPJsonRequest();
        req.setApplication_id("ЛЕНИН");
        req.setArticle_id("ТОВАРИЩИ!");
        req.setJournal_id("x");
        req.setPackage_location("x");
        req.setPackage_name("x");
        req.setPackage_state(PackageState.language_editing);
        req.setRequest_id(UUID.randomUUID().toString().replace("-", ""));
        req.setTimestamp(df.parse("2017-06-22T10:01:45+00:00"));
        req.setUser_name("x");
        req.setYear("x");
        // "2017-06-22T10:01:45+00:00"
        
        final byte[] bytes = mapper.writerWithDefaultPrettyPrinter().writeValueAsBytes(req);
        for (final byte b : bytes) {
            System.out.print((char) b);
        }
        
        if ("".length() == 1) {
            final Channel channel = null;
            MessageProperties.PERSISTENT_TEXT_PLAIN.toString();
            
            channel.queueDeclare(AMPP_IN, true, false, false, null);
            channel.queueDeclare(AMPP_STATUS, true, false, false, null);
            channel.queueDeclare(AMPP_OUT, true, false, false, null);
            
            channel.basicPublish("", AMPP_IN, JSON, bytes);
            
            // final channel.basicc
        }
    }
    
    private void disposeQueue(final Connection connection, final String queueName) {
        try {
            final Channel tmp = connection.createChannel();
            tmp.queueDelete(queueName);
            tmp.close();
        } catch (final Exception e2) {
        }
    }
    
    private void doWork(final String task) throws InterruptedException {
        for (final char ch : task.toCharArray()) {
            if (ch == '.') {
                Thread.sleep(1000);
            }
        }
    }
    
    private final static String AMPP_IN = "ampp_in"; //
    
    private final static String AMPP_STATUS = "ampp_status";
    
    private final static String AMPP_OUT = "ampp_out";
    
    private final static String EXCHANGE_NAME = "iacdc-dev-exchange";
    
    private final static String CONTENT_TYPE2 = "application/x.iacdc-ampp-request_1.0+json";
    
    /** Content-type "application/json", deliveryMode 1 (nonpersistent), priority zero */
    public static final BasicProperties JSON = new BasicProperties("application/json", null, null,
            1, 0, null, null, null, null, null, null, null, null, null);
}
