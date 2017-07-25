package org.foo.iacdc;

import java.beans.PropertyDescriptor;
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
import com.rabbitmq.client.Address;
import com.rabbitmq.client.BuiltinExchangeType;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.Consumer;
import com.rabbitmq.client.DefaultConsumer;
import com.rabbitmq.client.Envelope;
import com.rabbitmq.client.MessageProperties;
import com.rabbitmq.tools.json.JSONWriter;
import com.spr.ajwf.methods.jobs.DummyForPrefs;

public class MQTest extends TestCase {
    
    public static final String KEY_MSG_NODE_ID = "msgNodeId";
    
    public static final String CLIENT_KIND = "MQTest";
    
    private void commonSetup(final Channel channel, final String exchangeName) throws IOException {
        final boolean durable = true;
        channel.exchangeDeclare(exchangeName, BuiltinExchangeType.TOPIC, durable);
        channel.queueDeclare(AMPP_IN, durable, false, false, null);
        channel.queueBind(AMPP_IN, exchangeName, AMPP_IN);
    }
    
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
        final String user = System.getProperty("iampp.user");
        if (user != null) {
            factory.setUsername(user);
        }
        final String password = System.getProperty("iampp.password");
        if (password != null) {
            factory.setPassword(password);
        }
        final Address[] addrs = new Address[] { new Address(System.getProperty("iampp.host")) };
        final Connection connection = factory.newConnection(addrs);
        
        Channel channel = connection.createChannel();
        
        commonSetup(channel, exchangeName);
        
        final Preferences userPrefs = Preferences.userNodeForPackage(DummyForPrefs.class);
        msgNodeId = userPrefs.get(KEY_MSG_NODE_ID, null);
        
        if (isExclConsume) {
            for (boolean queueIsNew = false;;) {
                
                if (msgNodeId == null) {
                    queueIsNew = true;
                    msgNodeId = RandomStringUtils.randomAlphanumeric(8);
                }
                final String queueName = AMPP_LOCK + "." + msgNodeId + "." + clientKind;
                
                try {
                    
                    channel.queueDeclare(queueName, NON_DURABLE, EXCLUSIVE, AUTO_DELETE, null);
                    
                    if (queueIsNew) {
                        userPrefs.put(KEY_MSG_NODE_ID, msgNodeId);
                    }
                    break;
                } catch (final IOException e) {
                    // channel was implicitly closed
                    channel = null;
                    
                    if (queueIsNew) {
                        channel = disposeQueue(connection, queueName);
                    }
                    if (queueIsNew || !isExclConsume) {
                        throw e;
                    }
                    
                    channel = connection.createChannel();
                    msgNodeId = null;
                }
            }
        }
        
        final Consumer responseConsumer = new DefaultConsumer(channel) {
            
            @Override
            public void handleDelivery(final String consumerTag, final Envelope envelope,
                    final AMQP.BasicProperties properties, final byte[] body) throws IOException {
                final String message = new String(body, "UTF-8");
                System.out.println(" [x] Received '" + message + "'");
                try {
                    doWork(message);
                } catch (final InterruptedException e) {
                    System.out.println(" [x] Interrupted");
                } finally {
                    System.out.println(" [x] Done");
                    getChannel().basicAck(envelope.getDeliveryTag(), false);
                }
            }
        };
        
        final String outQueueAndKey = AMPP_OUT + "." + msgNodeId + "." + clientKind;
        
        channel.queueDeclare(outQueueAndKey, DURABLE, NON_EXCLUSIVE, NON_AUTO_DELETE, null);
        channel.queueBind(outQueueAndKey, exchangeName, outQueueAndKey);
        
        final boolean isAutoAck = false;
        
        channel.basicConsume(outQueueAndKey, isAutoAck, "", true, isExclConsume, null,
                responseConsumer);
        
        if ("".length() == 1) {
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
        req.setIn_package_location("x");
        req.setIn_package_name("x");
        req.setPackage_state(PackageState.language_editing);
        req.setRequest_id(UUID.randomUUID().toString().replace("-", ""));
        req.setTimestamp(df.parse("2017-06-22T10:01:45+00:00"));
        req.setUser_name(System.getProperty("user.name"));
        req.setYear("x");
        final PropertyDescriptor aid = null;
        // "2017-06-22T10:01:45+00:00"
        
        final byte[] bytes = mapper.writerWithDefaultPrettyPrinter().writeValueAsBytes(req);
        for (final byte b : bytes) {
            System.out.print((char) b);
        }
        System.out.print('\n');
        
        final JSONWriter wr = new JSONWriter(true); // TODO: use
        System.out.println(wr.write(req));
        
        if ("".length() == 1) {
            MessageProperties.PERSISTENT_TEXT_PLAIN.toString();
            
            channel.basicPublish("", AMPP_IN, JSON, bytes);
            
            // final channel.basicc
        }
    }
    
    private Channel disposeQueue(final Connection connection, final String queueName) {
        Channel chForClose = null;
        try {
            chForClose = connection.createChannel();
            chForClose.queueDelete(queueName);
        } catch (final Exception e2) {
            System.err.println("failed to delete new queue: " + e2.toString());
        }
        return chForClose;
    }
    
    private void doWork(final String task) throws InterruptedException {
        for (final char ch : task.toCharArray()) {
            if (ch == '.') {
                Thread.sleep(1000);
            }
        }
    }
    
    private static final boolean NON_DURABLE = false;
    
    private static final boolean DURABLE = true;
    
    private static final boolean EXCLUSIVE = true;
    
    private static final boolean NON_EXCLUSIVE = false;
    
    private static final boolean AUTO_DELETE = true;
    
    private static final boolean NON_AUTO_DELETE = false;
    
    private final static String AMPP_IN = "ampp-in";
    
    private final static String AMPP_STATUS = "ampp-status";
    
    private final static String AMPP_OUT = "ampp-out";
    
    private final static String AMPP_LOCK = "_ampp-lock";
    
    private final static String EXCHANGE_NAME = "iacdc-dev-exchange";
    
    private final static String CONTENT_TYPE2 = "application/x.iacdc-ampp-request_1.0+json";
    
    /** Content-type "application/json", deliveryMode 1 (nonpersistent), priority zero */
    public static final BasicProperties JSON = new BasicProperties("application/json", null, null,
            1, 0, null, null, null, null, null, null, null, null, null);
}
