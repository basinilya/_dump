package org.foo.iacdc;

public final class RabbitMQConstants {
    
    private RabbitMQConstants() {}
    
    public static final boolean NON_AUTO_ACK = false;
    
    public static final boolean NON_DURABLE = false;
    
    public static final boolean DURABLE = true;
    
    public static final boolean EXCLUSIVE = true;
    
    public static final boolean NON_EXCLUSIVE = false;
    
    public static final boolean AUTO_DELETE = true;
    
    public static final boolean NON_AUTO_DELETE = false;
    
    public static final String OPT_DLE = "x-dead-letter-exchange";
}
