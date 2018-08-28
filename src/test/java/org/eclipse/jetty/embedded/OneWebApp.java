//
//  ========================================================================
//  Copyright (c) 1995-2015 Mort Bay Consulting Pty. Ltd.
//  ------------------------------------------------------------------------
//  All rights reserved. This program and the accompanying materials
//  are made available under the terms of the Eclipse Public License v1.0
//  and Apache License v2.0 which accompanies this distribution.
//
//      The Eclipse Public License is available at
//      http://www.eclipse.org/legal/epl-v10.html
//
//      The Apache License v2.0 is available at
//      http://www.opensource.org/licenses/apache2.0.php
//
//  You may elect to redistribute this code under either of these licenses.
//  ========================================================================
//
 
package org.eclipse.jetty.embedded;
 
import java.io.File;
import java.net.URL;
import java.net.URLClassLoader;

import org.apache.jasper.servlet.JspServlet;
import org.eclipse.jetty.server.Handler;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.HandlerCollection;
import org.eclipse.jetty.servlet.DefaultServlet;
import org.eclipse.jetty.servlet.ServletHolder;
import org.eclipse.jetty.webapp.WebAppContext;
 
public class OneWebApp
{
    public static void main( String[] args ) throws Exception
    {
        Server server = new Server(8080);
        
        WebAppContext context = new WebAppContext();
        context.setContextPath("/");
        context.setWar("src/main/webapp/");

        File workDir = new File("target/work");
        context.setTempDirectory(workDir);
        // context.setAttribute("javax.servlet.context.tempdir",scratchDir);
        
     // Set Classloader of Context to be sane (needed for JSTL)
     // JSP requires a non-System classloader, this simply wraps the
     // embedded System classloader in a way that makes it suitable
     // for JSP to use
        ClassLoader jspClassLoader = new URLClassLoader(new URL[0], OneWebApp.class.getClassLoader());
        context.setClassLoader(jspClassLoader);
        /*
        */
        
     // Add JSP Servlet (must be named "jsp")
        ServletHolder holderJsp = new ServletHolder("jsp",JspServlet.class);
        holderJsp.setInitOrder(0);
        /*
        */

        
     // Add Default Servlet (must be named "default")
        /*
        ServletHolder holderDefault = new ServletHolder("default",DefaultServlet.class);
        holderDefault.setInitParameter("relativeResourceBase","src/main/webapp/");
        holderDefault.setInitParameter("dirAllowed","true");
        context.addServlet(holderDefault,"/");
        */
        
        HandlerCollection hc = new HandlerCollection();
        hc.setHandlers(new Handler[] {context});
        
        server.setHandler(hc);
        server.setStopAtShutdown(true);
        server.start();
        
        server.join();
        
        // server.removeBean(o);
        // server.addBean(o);
    }
}