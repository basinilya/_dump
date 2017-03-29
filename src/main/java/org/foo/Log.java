package org.foo;

import java.io.PrintWriter;
import java.io.StringWriter;

public class Log {
    
    public static void log(final Object o) {
        final Thread t = Thread.currentThread();
        System.out.println(t.getId() + " " + t.getName() + " " + o);
    }
    
    public static void log(final Object o, final Throwable e) {
        final StringWriter sw = new StringWriter();
        final PrintWriter out = new PrintWriter(sw);
        e.printStackTrace(out);
        log(o + " " + sw.toString());
    }
}
