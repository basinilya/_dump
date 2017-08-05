package org.foo.iacdc;

import java.io.IOException;
import java.util.concurrent.TimeoutException;

import org.apache.commons.io.IOUtils;

import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.MessageProperties;
import com.rabbitmq.tools.json.JSONReader;
import com.rabbitmq.tools.json.JSONWriter;

public class NewTask {
    
    private final static String TASK_QUEUE_NAME = "acdc-in.qd5F19Nw.FTPExchangeJob.3";
    
    public static void main(final String[] argv) throws IOException, TimeoutException {
        String message = null;
        byte[] bytes =
                IOUtils.toByteArray(NewTask.class.getResourceAsStream("examples/ampp1.json"));
        message = new String(bytes, "UTF-8");
        final Object map = new JSONReader().read(message);
        message = new JSONWriter().write(map);
        bytes = message.getBytes("UTF-8");
        // System.exit(0);
        
        final ConnectionFactory factory = new ConnectionFactory();
        factory.setHost("dioptase");
        final Connection connection = factory.newConnection();
        final Channel channel = connection.createChannel();
        
        // channel.queueDeclarePassive(TASK_QUEUE_NAME);
        
        // channel.queueDeclare(TASK_QUEUE_NAME, true, false, false, null);
        // message = getMessage(argv);
        
        channel.basicPublish("", TASK_QUEUE_NAME, MessageProperties.PERSISTENT_TEXT_PLAIN, bytes);
        System.out.println(" [x] Sent '" + message + "'");
        
        channel.close();
        connection.close();
    }
    
    private static String getMessage(final String[] strings) {
        if (strings.length < 1) {
            return "Hello World!";
        }
        return joinStrings(strings, " ");
    }
    
    private static String joinStrings(final String[] strings, final String delimiter) {
        final int length = strings.length;
        if (length == 0) {
            return "";
        }
        final StringBuilder words = new StringBuilder(strings[0]);
        for (int i = 1; i < length; i++) {
            words.append(delimiter).append(strings[i]);
        }
        return words.toString();
    }
}
