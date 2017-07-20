package org.foo.iacdc;

import java.nio.file.attribute.FileTime;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.TimeZone;
import java.util.UUID;

import junit.framework.TestCase;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.rabbitmq.client.AMQP.BasicProperties;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.MessageProperties;

public class MQTest extends TestCase {
    
    public void testDoIt() throws Exception {
        
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
        System.out.println(Calendar.getInstance());
        System.out.println(FileTime.fromMillis(System.currentTimeMillis()));
        
        final byte[] bytes = mapper.writerWithDefaultPrettyPrinter().writeValueAsBytes(req);
        for (final byte b : bytes) {
            System.out.print((char) b);
        }
        
        if ("".length() == 0) {
            
            final ConnectionFactory factory = new ConnectionFactory();
            factory.setHost(System.getProperty("iampp.host"));
            factory.setUsername(System.getProperty("iampp.user"));
            factory.setPassword(System.getProperty("iampp.password"));
            final Connection connection = factory.newConnection();
            MessageProperties.PERSISTENT_TEXT_PLAIN.toString();
            
            final Channel channel = connection.createChannel();
            
            channel.queueDeclare(QUEUE_NAME, true, false, false, null);
            
            channel.basicPublish("", QUEUE_NAME, JSON, bytes);
        }
    }
    
    private final static String QUEUE_NAME = "ampp-in"; //
    
    private final static String CONTENT_TYPE2 = "application/x.iacdc-ampp-request_1.0+json";
    
    /** Content-type "application/json", deliveryMode 1 (nonpersistent), priority zero */
    public static final BasicProperties JSON = new BasicProperties("application/json", null, null,
            1, 0, null, null, null, null, null, null, null, null, null);
}
