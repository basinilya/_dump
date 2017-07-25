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
                public synchronized void handleDelivery(final String consumerTag,
                        final Envelope envelope, final AMQP.BasicProperties properties,
                        final byte[] body) throws IOException {
                    System.out.println("" + Thread.currentThread().getName());
                    final String message = new String(body, "UTF-8");
                    System.out.println(" [x] Received '" + message + "'");
                    try {
                        doWork(message);
                        System.out.println("basicCancel");
                        getChannel().basicCancel(consumerTag);
                        System.out.println("basicCancel done");
                        Thread.sleep(3000);
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
            consumerTag =
                    channel.basicConsume(TASK_QUEUE_NAME, isAutoAck, "", true, exclusive, null,
                            consumer);
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
    
}
