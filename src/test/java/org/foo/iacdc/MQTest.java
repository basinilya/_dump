package org.foo.iacdc;

import static org.foo.iacdc.RabbitMQConstants.*;

import java.beans.PropertyDescriptor;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
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
        channel.exchangeDeclare(exchangeName, BuiltinExchangeType.TOPIC, DURABLE);
        channel.queueDeclare(AMPP_IN, DURABLE, false, false, null);
        channel.queueBind(AMPP_IN, exchangeName, AMPP_IN);
    }
    
    public void testDoIt() throws Exception {
        
        final String user = System.getProperty("iampp.user");
        final String password = System.getProperty("iampp.password");
        final Address[] addrs = new Address[] { new Address(System.getProperty("iampp.host")) };
        
        final int nThreads = 5;
        final String exchangeName = EXCHANGE_NAME;
        String msgNodeId = null;
        
        boolean isExclConsume = true;
        final String clientKind = CLIENT_KIND;
        
        if (msgNodeId != null) {
            isExclConsume = false; // cluster
        }
        
        //
        final ArrayList<Channel> channels = new ArrayList<Channel>(nThreads);
        //
        final ConnectionFactory factory = new ConnectionFactory();
        if (user != null) {
            factory.setUsername(user);
        }
        if (password != null) {
            factory.setPassword(password);
        }
        final Connection connection = factory.newConnection(addrs);
        
        Channel channel = connection.createChannel();
        channels.add(channel);
        
        commonSetup(channel, exchangeName);
        
        final Preferences userPrefs = Preferences.userNodeForPackage(DummyForPrefs.class);
        msgNodeId = userPrefs.get(KEY_MSG_NODE_ID, null);
        
        if (isExclConsume) {
            for (boolean nodeIsNew = false;;) {
                
                if (msgNodeId == null) {
                    nodeIsNew = true;
                    msgNodeId = RandomStringUtils.randomAlphanumeric(8);
                }
                final String queueName = AMPP_LOCK + "." + msgNodeId + "." + clientKind;
                
                try {
                    
                    channel.queueDeclare(queueName, NON_DURABLE, EXCLUSIVE, AUTO_DELETE, null);
                    
                    if (nodeIsNew) {
                        userPrefs.put(KEY_MSG_NODE_ID, msgNodeId);
                    }
                    break;
                } catch (final IOException e) {
                    // channel was implicitly closed
                    channels.clear();
                    
                    if (nodeIsNew) {
                        throw e;
                    }
                    
                    channel = connection.createChannel();
                    channels.add(channel);
                    msgNodeId = null;
                }
            }
        }
        
        final String outQueueAndKey = AMPP_OUT + "." + msgNodeId + "." + clientKind;
        
        channel.queueDeclare(outQueueAndKey, DURABLE, NON_EXCLUSIVE, NON_AUTO_DELETE, null);
        channel.queueBind(outQueueAndKey, exchangeName, outQueueAndKey);
        
        for (int i = 0;; i++) {
            channel.basicQos(1);
            
            final Consumer responseConsumer = new DefaultConsumer(channel) {
                
                @Override
                public void handleDelivery(final String consumerTag, final Envelope envelope,
                        final AMQP.BasicProperties properties, final byte[] body)
                        throws IOException {
                    final String message = new String(body, "UTF-8");
                    
                    System.out.println(" [x] Received '" + message + "'" + properties.toString()
                            + ' ' + envelope.toString());
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
            
            channel.basicConsume(outQueueAndKey, NON_AUTO_ACK, responseConsumer);
            
            if ((++i) >= nThreads) {
                break;
            }
            
            channel = connection.createChannel();
            channels.add(channel);
        }
        
        System.out.println("" + outQueueAndKey);
        
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
    
    private void doWork(final String task) throws InterruptedException {
        for (final char ch : task.toCharArray()) {
            if (ch == '.') {
                Thread.sleep(1000);
            }
        }
    }
    
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
