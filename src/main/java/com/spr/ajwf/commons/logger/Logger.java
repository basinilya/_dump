package com.spr.ajwf.commons.logger;

import java.io.PrintWriter;
import java.io.StringWriter;

public class Logger {
    
    public Logger(final Class<?> clazz) {}
    
    public void info(final Object o) {
        LogTest.log(o);
    }
    
    public void info(final Object o, final Throwable e) {
        log(o, e);
    }
    
    public void error(final Object o) {
        LogTest.log(o);
    }
    
    public void error(final Object o, final Throwable e) {
        log(o, e);
    }
    
    public void trace(final Object o, final Throwable e) {
        log(o, e);
    }
    
    public void trace(final Object o) {
        LogTest.log(o);
    }
    
    public static void log(final Object o, final Throwable e) {
        final StringWriter sw = new StringWriter();
        final PrintWriter out = new PrintWriter(sw);
        e.printStackTrace(out);
        LogTest.log(o + " " + sw.toString());
    }
}
