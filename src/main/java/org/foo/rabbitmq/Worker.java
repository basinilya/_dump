package org.foo.rabbitmq;

import java.io.IOException;
import java.util.concurrent.TimeoutException;

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
        
        final ConnectionFactory factory = new ConnectionFactory();
        factory.setHost("dioptase");
        final Connection connection = factory.newConnection();
        final Channel channel = connection.createChannel();
        
        channel.queueDeclare(TASK_QUEUE_NAME, true, false, false, null);
        System.out.println(" [*] Waiting for messages. To exit press CTRL+C");
        
        final int prefetchCount = 1;
        channel.basicQos(prefetchCount); // accept only one unack-ed message at a time (see below)
        
        final Consumer consumer = new DefaultConsumer(channel) {
            
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
                    channel.basicAck(envelope.getDeliveryTag(), false);
                }
            }
        };
        final boolean autoAck = false; // acknowledgment is covered below
        channel.basicConsume(TASK_QUEUE_NAME, autoAck, consumer);
        
    }
    
    private static void doWork(final String task) throws InterruptedException {
        for (final char ch : task.toCharArray()) {
            if (ch == '.') {
                Thread.sleep(1000);
            }
        }
    }
    
}
