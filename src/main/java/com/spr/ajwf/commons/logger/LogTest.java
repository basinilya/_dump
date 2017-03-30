package com.spr.ajwf.commons.logger;

final class LogTest {
    
    private LogTest() {}
    
    public static void log(final Object o) {
        final Thread t = Thread.currentThread();
        System.out.println(t.getId() + " " + t.getName() + " " + o);
    }
    
}
