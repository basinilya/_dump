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

public class Recv {
    
    private final static String QUEUE_NAME = "hello";
    
    public static void main(final String[] argv) throws IOException, InterruptedException,
            TimeoutException {
        
        final ConnectionFactory factory = new ConnectionFactory();
        factory.setHost("dioptase");
        final Connection connection = factory.newConnection();
        final Channel channel = connection.createChannel();
        
        channel.queueDeclare(QUEUE_NAME, false, false, false, null);
        System.out.println(" [*] Waiting for messages. To exit press CTRL+C");
        
        final Consumer consumer = new DefaultConsumer(channel) {
            
            @Override
            public void handleDelivery(final String consumerTag, final Envelope envelope,
                    final AMQP.BasicProperties properties, final byte[] body) throws IOException {
                final String message = new String(body, "UTF-8");
                System.out.println(" [x] Received '" + message + "'");
            }
        };
        channel.basicConsume(QUEUE_NAME, true, consumer);
        
    }
    
}
