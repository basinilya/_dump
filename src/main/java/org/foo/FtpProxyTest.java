package org.foo;

import java.net.InetSocketAddress;
import java.net.Proxy;
import java.net.ProxySelector;
import java.net.URI;
import java.util.Iterator;
import java.util.List;

import org.apache.commons.net.ftp.FTPClient;

public class FtpProxyTest {
    
    public static void main(final String[] args) throws Exception {
        if ("".length() == 0) {
            final FTPClient asd = null;
            asd.connect("");
            asd.setSoTimeout(11);
            ProxySelector ps = null;
            ps = ProxySelector.getDefault();
            final ProxySelector parent = ps;
            // ps = new DefaultProxySelector(parent);
            URI u = null;
            u = new URI("socket://bs.reksoft.ru:1489");
            u = new URI("socket://localhost:1489");
            u = new URI("socket://192.168.0.1:1489");
            u = new URI("socket://127.0.0.1:1489");
            u = new URI("http://123.0.0.2:1489");
            u = new URI("socket://192.168.148.150:1489");
            final List<Proxy> l = ps.select(u);
            for (final Iterator<Proxy> iter = l.iterator(); iter.hasNext();) {
                
                final Proxy proxy = iter.next();
                
                System.out.println("proxy type : " + proxy.type());
                
                final InetSocketAddress addr = (InetSocketAddress) proxy.address();
                
                if (addr == null) {
                    
                    System.out.println("No Proxy");
                    
                } else {
                    System.out.println("proxy hostname : " + addr.getHostName());
                    System.out.println("proxy port : " + addr.getPort());
                }
            }
        }
        // new Aaa().aaa();
    }
    
    public void aaa() throws Exception {
        // java.net.useSystemProxies
        // ProxySelector asd;
        
        String server, login, pass, dstdir;
        {
            // vsftpd
            server = "192.168.56.150";
            login = "anonymous";
            pass = "a@b.org";
            dstdir = "/pub";
        }
        
        {
            // BulletProof FTP Server
            // System.setProperty("socksProxyHost", "proxy.reksoft.ru");
            // System.setProperty("socksProxyPort", "1080");
            server = "103.82.76.10";
            login = "ACDCIntegrationTest";
            pass = "05nBcRP";
            dstdir = "/test";
        }
        
        {
            // EFT Server
            // System.setProperty("socksProxyHost", "proxy.reksoft.ru");
            // System.setProperty("socksProxyPort", "1080");
            server = "ftp.springer-sbm.com";
            login = "spca1";
            pass = "welkom";
            dstdir = "/u02/acdc/work/ImportDeliveryArchive/test";
        }
        
        {
            // Pure-FTPd
            // System.setProperty("socksProxyHost", "proxy.reksoft.ru");
            // System.setProperty("socksProxyPort", "1080");
            server = "ftpupload.movingimage24.de"; // ;
            login = "2132F5694fe31202fb";
            pass = "Vv8S6q247fvBljHl";
            dstdir = "/test";
        }
        
    }
}
