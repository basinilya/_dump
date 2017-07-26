package org.foo.iacdc;

import static org.foo.iacdc.RabbitMQConstants.*;

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.TimeZone;
import java.util.concurrent.ExecutorService;
import java.util.prefs.Preferences;

import org.apache.commons.lang.RandomStringUtils;

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
import com.rabbitmq.tools.json.JSONReader;
import com.rabbitmq.tools.json.JSONWriter;
import com.spr.ajwf.methods.jobs.DummyForPrefs;

public class MQListener {
    
    public static final String KEY_MSG_NODE_ID = "msgNodeId";
    
    private void commonSetup(final Channel channel, final String exchangeName) throws IOException {
        channel.exchangeDeclare(exchangeName, BuiltinExchangeType.TOPIC, DURABLE);
        channel.queueDeclare(AMPP_IN, DURABLE, false, false, null);
        channel.queueBind(AMPP_IN, exchangeName, AMPP_IN);
    }
    
    public void start(final Address[] addrs, final String user, final String password,
            final String exchangeName, final String clientKind, final String msgNodeIdArg,
            final ExecutorService sharedExecutor, final int nThreads) throws Exception {
        //
        String msgNodeId = msgNodeIdArg;
        
        boolean isExclConsume = true;
        
        Preferences userPrefs = null;
        if (msgNodeId != null) {
            isExclConsume = false; // cluster
        } else {
            userPrefs = Preferences.userNodeForPackage(DummyForPrefs.class);
            msgNodeId = userPrefs.get(KEY_MSG_NODE_ID, null);
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
        final Connection connection = factory.newConnection(sharedExecutor, addrs);
        
        // create first channel
        Channel channel = connection.createChannel();
        channels.add(channel);
        
        // declare queues and exchanges common to AMPP and ACDC
        commonSetup(channel, exchangeName);
        
        // setup pseudo-scheduling
        final HashMap<String, Object> dleToDefaultExchange = new HashMap<String, Object>();
        dleToDefaultExchange.put(OPT_DLE, "");
        channel.queueDeclare(ACDC_RETRY, DURABLE, NON_EXCLUSIVE, NON_AUTO_DELETE,
                dleToDefaultExchange);
        channel.exchangeDeclare(ACDC_RETRY, BuiltinExchangeType.FANOUT, DURABLE, NON_AUTO_DELETE,
                null);
        
        // Check if we can resume consuming responses
        if (isExclConsume) {
            for (boolean nodeIsNew = false;;) {
                
                if (msgNodeId == null) {
                    nodeIsNew = true;
                    msgNodeId = RandomStringUtils.randomAlphanumeric(8);
                }
                final String queueName = ACDC_LOCK_PREF + msgNodeId + "." + clientKind;
                
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
        
        // declare queue for responses
        final String outQueueAndKey = ACDC_IN_PREF + msgNodeId + "." + clientKind;
        channel.queueDeclare(outQueueAndKey, DURABLE, NON_EXCLUSIVE, NON_AUTO_DELETE, null);
        channel.queueBind(outQueueAndKey, exchangeName, outQueueAndKey);
        
        /*
         * final ObjectMapper mapper = new ObjectMapper(); mapper.setDateFormat(df); final byte[]
         * bytes = mapper.writerWithDefaultPrettyPrinter().writeValueAsBytes(req);
         */
        
        // create parallel consumers
        for (int i = 0;; i++) {
            channel.basicQos(1);
            
            final Consumer responseConsumer = new DefaultConsumer(channel) {
                
                @Override
                public void handleDelivery(final String consumerTag, final Envelope envelope,
                        final AMQP.BasicProperties properties, final byte[] body)
                        throws IOException {
                    final String message = new String(body, ENCODING);
                    
                    System.out.println(" [x] Received '" + message + "'" + properties.toString()
                            + ' ' + envelope.toString());
                    
                    // setup JSON
                    final SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ssXXX");
                    df.setTimeZone(TimeZone.getTimeZone("UTC"));
                    final JSONWriter wr = new JSONWriter(true);
                    final JSONReader rd = new JSONReader();
                    
                    boolean canRetry = false;
                    AMPPJsonResponse response = null;
                    int tryNo = 0;
                    try {
                        
                        @SuppressWarnings("unchecked")
                        final Map<String, Object> map = (Map<String, Object>) rd.read(message);
                        response = readObject(AMPPJsonResponse.class, map, null);
                        if (response.getTry_no() != null) {
                            tryNo = response.getTry_no();
                        }
                        canRetry = true;
                        doWork(message);
                        getChannel().basicAck(envelope.getDeliveryTag(), false);
                    } catch (final Exception e) {
                        System.out.println(" failed " + e);
                        if (canRetry) {
                            final int expiration = 60000;
                            final BasicProperties newProps =
                                    properties.builder().expiration(Integer.toString(expiration))
                                            .build();
                            response.setTry_no(tryNo + 1);
                            final byte[] retrybody = wr.write(response).getBytes(ENCODING);
                            getChannel().basicPublish(ACDC_RETRY, envelope.getRoutingKey(),
                                    newProps, retrybody);
                            getChannel().basicAck(envelope.getDeliveryTag(), false);
                        } else {
                            getChannel().basicReject(envelope.getDeliveryTag(), false);
                        }
                    } finally {
                        System.out.println(" [x] Done");
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
        
        if ("".length() == 1) {
            MessageProperties.PERSISTENT_TEXT_PLAIN.toString();
            
            channel.basicPublish("", AMPP_IN, PERSISTENT_JSON, null);
        }
    }
    
    protected void doWork(final String task) throws Exception {
        for (final char ch : task.toCharArray()) {
            if (ch == '.') {
                Thread.sleep(1000);
            }
        }
    }
    
    @SuppressWarnings("unchecked")
    public <T> T readObject(final Class<T> klass, final Map<String, Object> input,
            final String[] properties) {
        Set<String> propertiesSet = null;
        if (properties != null) {
            propertiesSet = new HashSet<String>();
            for (final String p : properties) {
                propertiesSet.add(p);
            }
        }
        
        BeanInfo info;
        try {
            info = Introspector.getBeanInfo(klass);
        } catch (final IntrospectionException ie) {
            info = null;
        }
        
        Object object;
        try {
            object = klass.newInstance();
        } catch (final Exception e1) {
            return null;
        }
        
        if (info != null) {
            final PropertyDescriptor[] props = info.getPropertyDescriptors();
            for (int i = 0; i < props.length; ++i) {
                final PropertyDescriptor prop = props[i];
                final String name = prop.getName();
                if (propertiesSet == null && name.equals("class")) {
                    // We usually don't want the class in there.
                    continue;
                }
                if (propertiesSet == null || propertiesSet.contains(name)) {
                    final Method accessor = prop.getWriteMethod();
                    if (accessor != null && !Modifier.isStatic(accessor.getModifiers())) {
                        try {
                            final Object val = input.get(prop.getName());
                            accessor.invoke(object, val);
                        } catch (final Exception e) {
                            // Ignore it.
                        }
                    }
                }
            }
        }
        
        final Field[] ff = object.getClass().getDeclaredFields();
        for (int i = 0; i < ff.length; ++i) {
            final Field field = ff[i];
            final int fieldMod = field.getModifiers();
            final String name = field.getName();
            if (propertiesSet == null || propertiesSet.contains(name)) {
                if (!Modifier.isStatic(fieldMod)) {
                    try {
                        final Object value = input.get(field.getName());
                        field.set(object, value);
                    } catch (final Exception e) {
                        // Ignore it.
                    }
                }
            }
        }
        
        return (T) object;
    }
    
    private final static String ENCODING = "UTF-8";
    
    private final static String AMPP_IN = "ampp-in";
    
    /** Prefix for the name of the queue for external systems replies */
    private final static String ACDC_IN_PREF = "acdc-in.";
    
    /** global exchange and queue names for retry messages */
    private final static String ACDC_RETRY = "acdc-retry";
    
    /** Prefix for the name of the technical, always empty queue for locking */
    private final static String ACDC_LOCK_PREF = "_acdc-lock.";
    
    // private final static String CONTENT_TYPE2 = "application/x.iacdc-ampp-request_1.0+json";
    
    public static final BasicProperties PERSISTENT_JSON = MessageProperties.PERSISTENT_TEXT_PLAIN
            .builder().contentType("application/json").build();
}
