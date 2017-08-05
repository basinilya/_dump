package org.foo.iacdc;

import static com.spr.ajwf.methods.jobs.iacdc.RabbitMQConstants.*;

import java.io.IOException;
import java.util.HashMap;
import java.util.concurrent.TimeoutException;

import org.apache.commons.io.IOUtils;

import com.rabbitmq.client.BuiltinExchangeType;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.MessageProperties;
import com.rabbitmq.tools.json.JSONReader;
import com.rabbitmq.tools.json.JSONWriter;

public class CopyOfNewTask {
    
    private final static String TASK_QUEUE_NAME = "acdc-in.qd5F19Nw.FTPExchangeJob.3";
    
    public static final String ACDC_RETRY = "acdc-retry";
    
    private static void declareRetryQueue(final Channel channel) throws IOException {
        final HashMap<String, Object> dleToDefaultExchange = new HashMap<String, Object>();
        dleToDefaultExchange.put(OPT_DLE, "");
        channel.queueDelete("acdc-in.qd5F19Nw.FTPExchangeJob.3");
        channel.queueDeclare(ACDC_RETRY, DURABLE, NON_EXCLUSIVE, NON_AUTO_DELETE,
                dleToDefaultExchange);
        channel.exchangeDeclare(ACDC_RETRY, BuiltinExchangeType.FANOUT, DURABLE, NON_AUTO_DELETE,
                null);
        channel.queueBind(ACDC_RETRY, ACDC_RETRY, "");
        System.exit(0);
        // channel.queueUnbind(ACDC_RETRY, ACDC_RETRY, "");
    }
    
    public static void main(final String[] argv) throws IOException, TimeoutException {
        String message = null;
        byte[] bytes =
                IOUtils.toByteArray(CopyOfNewTask.class.getResourceAsStream("examples/ampp1.json"));
        message = new String(bytes, "UTF-8");
        final Object map = new JSONReader().read(message);
        message = new JSONWriter().write(map);
        bytes = message.getBytes("UTF-8");
        
        // System.exit(0);
        
        final ConnectionFactory factory = new ConnectionFactory();
        factory.setHost("dioptase");
        final Connection connection = factory.newConnection();
        final Channel channel = connection.createChannel();
        
        declareRetryQueue(channel);
        
        // channel.queueDeclarePassive(TASK_QUEUE_NAME);
        
        // channel.queueDeclare(TASK_QUEUE_NAME, true, false, false, null);
        // message = getMessage(argv);
        
        // channel.basicPublish("acdc-retry", "", MessageProperties.PERSISTENT_TEXT_PLAIN, bytes);
        // channel.basicPublish("",
        // "acdc-in.qd5F19Nw.FTPExchangeJob.3",MessageProperties.PERSISTENT_TEXT_PLAIN, bytes);
        channel.basicPublish(ACDC_RETRY, "acdc-in.qd5F19Nw.FTPExchangeJob.3",
                MessageProperties.PERSISTENT_TEXT_PLAIN.builder().expiration("7000").build(), bytes);
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
