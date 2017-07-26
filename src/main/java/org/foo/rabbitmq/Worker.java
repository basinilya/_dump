package org.foo.rabbitmq;

import java.io.IOException;
import java.util.HashMap;
import java.util.concurrent.TimeoutException;

import com.rabbitmq.client.AMQP;
import com.rabbitmq.client.AMQP.BasicProperties;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.Consumer;
import com.rabbitmq.client.DefaultConsumer;
import com.rabbitmq.client.Envelope;

public class Worker {
    
    private final static String RETRY_EXCHANGE = "retry_exchange";
    
    private final static String RETRY_QUEUE = "retry_queue";
    
    private final static String TASK_QUEUE = "task_queue";
    
    public static void main(final String[] argv) throws IOException, InterruptedException,
            TimeoutException {
        final ConnectionFactory factory = new ConnectionFactory();
        factory.setHost("dioptase");
        
        final Connection connection = factory.newConnection();
        
        final Channel channel = connection.createChannel();
        try {
            
            final HashMap<String, Object> queueArgs = new HashMap<String, Object>();
            queueArgs.put("x-dead-letter-exchange", "");
            // queueArgs.put("x-message-ttl", 15000);
            
            // channel.exchangeDelete(RETRY_EXCHANGE);
            // channel.queueDelete(RETRY_QUEUE);
            // channel.queueDelete(TASK_QUEUE);
            
            // channel.exchangeDeclare(RETRY_EXCHANGE, BuiltinExchangeType.FANOUT, NON_DURABLE,
            // AUTO_DELETE, null);
            
            channel.queueDeclare(RETRY_QUEUE, NON_DURABLE, NON_EXCLUSIVE, AUTO_DELETE, queueArgs);
            // channel.queueBind(RETRY_QUEUE, RETRY_EXCHANGE, "", null);
            
            channel.queueDeclare(TASK_QUEUE, NON_DURABLE, NON_EXCLUSIVE, AUTO_DELETE, null);
            
            final BasicProperties props =
                    new BasicProperties.Builder().contentType("text/plain").deliveryMode(2)
                            .priority(0).expiration("5000").build();
            System.out.println(props.toString());
            
            channel.basicPublish(RETRY_EXCHANGE, TASK_QUEUE, props, "test-retry".getBytes("UTF-8"));
            System.out.println(" [*] Waiting for messages. To exit press CTRL+C");
            
            final int prefetchCount = 1;
            channel.basicQos(prefetchCount); // accept only one unack-ed message at a time (see
                                             // below)
            
            final Consumer consumer = new DefaultConsumer(channel) {
                
                @Override
                public void handleCancel(final String consumerTag) throws IOException {
                    System.out.println("close");
                    connection.close();
                    System.out.println("close done");
                }
                
                @Override
                public void handleCancelOk(final String consumerTag) {
                    try {
                        handleCancel(consumerTag);
                    } catch (final IOException e) {
                        e.printStackTrace();
                    }
                }
                
                @Override
                public void handleDelivery(final String consumerTag, final Envelope envelope,
                        final AMQP.BasicProperties properties, final byte[] body)
                        throws IOException {
                    System.out.println("" + Thread.currentThread().getName());
                    final String message = new String(body, "UTF-8");
                    System.out.println(" [x] Received '" + message + "'" + properties.toString()
                            + ' ' + envelope.toString());
                    try {
                        doWork(message);
                        if ("".length() == 0) {
                            System.out.println("basicCancel");
                            getChannel().basicCancel(consumerTag);
                            System.out.println("basicCancel done");
                        }
                        // Thread.sleep(3000);
                    } catch (final InterruptedException e) {
                        System.out.println(" [x] Interrupted");
                    } finally {
                        System.out.println("basicAck");
                        channel.basicAck(envelope.getDeliveryTag(), false);
                        System.out.println("basicAck done");
                    }
                }
            };
            final boolean isAutoAck = false; // acknowledgment is covered below
            // Start a non-nolocal, non-exclusive consumer, with a server-generated consumerTag.
            // channel.basicConsume(TASK_QUEUE_NAME, autoAck, consumer);
            final boolean exclusive = true;
            
            channel.basicConsume(TASK_QUEUE, isAutoAck, "", true, exclusive, null, consumer);
        } catch (final Exception e) {
            connection.close();
            throw e;
        }
        
        System.out.println(" [x] Main exit");
    }
    
    private static void doWork(final String task) throws InterruptedException {
        Thread.sleep(1500);
        for (final char ch : task.toCharArray()) {
            if (ch == '.') {
                Thread.sleep(1000);
            }
        }
    }
    
    public static final boolean NON_AUTO_ACK = false;
    
    public static final boolean NON_DURABLE = false;
    
    public static final boolean DURABLE = true;
    
    public static final boolean EXCLUSIVE = true;
    
    public static final boolean NON_EXCLUSIVE = false;
    
    public static final boolean AUTO_DELETE = true;
    
    public static final boolean NON_AUTO_DELETE = false;
}
