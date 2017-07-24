package org.foo.rabbitmq;

import java.io.IOException;
import java.util.concurrent.TimeoutException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.rabbitmq.client.AMQP;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.Consumer;
import com.rabbitmq.client.DefaultConsumer;
import com.rabbitmq.client.Envelope;
import com.rabbitmq.client.ShutdownSignalException;

public class Worker {
    
    private final static String TASK_QUEUE_NAME = "task_queue";
    
    public static void main(final String[] argv) throws IOException, InterruptedException,
            TimeoutException {
        final Logger logger = LoggerFactory.getLogger(Worker.class);
        logger.info("aaa");
        
        final ConnectionFactory factory = new ConnectionFactory();
        factory.setHost("dioptase");
        final Connection connection = factory.newConnection();
        final Channel channel = connection.createChannel();
        String consumerTag = null;
        try {
            
            channel.queueDeclare(TASK_QUEUE_NAME, true, false, false, null);
            System.out.println(" [*] Waiting for messages. To exit press CTRL+C");
            
            final int prefetchCount = 1;
            channel.basicQos(prefetchCount); // accept only one unack-ed message at a time (see
                                             // below)
            
            final Consumer consumer = new DefaultConsumer(channel) {
                
                @Override
                public void handleShutdownSignal(final String consumerTag,
                        final ShutdownSignalException sig) {
                    System.out.println("" + Thread.currentThread().getName());
                }
                
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
                        final long tag = envelope.getDeliveryTag();
                        channel.basicAck(tag, false);
                    }
                }
            };
            final boolean autoAck = false; // acknowledgment is covered below
            // Start a non-nolocal, non-exclusive consumer, with a server-generated consumerTag.
            // channel.basicConsume(TASK_QUEUE_NAME, autoAck, consumer);
            final boolean exclusive = true;
            consumerTag =
                    channel.basicConsume(TASK_QUEUE_NAME, autoAck, "", true, exclusive, null,
                            consumer);
        } catch (final Exception e) {
            connection.close();
            throw e;
        }
        
        Thread.sleep(1000);
        System.out.println("" + Thread.currentThread().getName());
        // final GetResponse gr = channel.basicGet(TASK_QUEUE_NAME, true);
        channel.basicCancel(consumerTag);
        Thread.sleep(5000);
        connection.close();
        System.out.println(" [x] Main exit");
    }
    
    private static void doWork(final String task) throws InterruptedException {
        Thread.sleep(5000);
        for (final char ch : task.toCharArray()) {
            if (ch == '.') {
                Thread.sleep(1000);
            }
        }
    }
    
}
