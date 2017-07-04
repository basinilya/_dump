package org.foo;

import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.net.InetSocketAddress;
import java.net.Proxy;
import java.net.ProxySelector;
import java.net.SocketAddress;
import java.net.URI;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

import sun.misc.REException;
import sun.misc.RegexpPool;
import sun.net.NetProperties;
import sun.net.SocksProxy;

public class DefaultProxySelector extends ProxySelector {
    
    static final String[][] props = {
            { "http", "http.proxy", "proxy", "socksProxy" },
            { "https", "https.proxy", "proxy", "socksProxy" },
            { "ftp", "ftp.proxy", "ftpProxy", "proxy", "socksProxy" },
            { "gopher", "gopherProxy", "socksProxy" },
            { "socket", "socksProxy" } };
    
    private static final String SOCKS_PROXY_VERSION = "socksProxyVersion";
    
    private boolean hasSystemProxies = false;
    
    private NonProxyInfo NonProxyInfo_ftpNonProxyInfo() {
        return NonProxyInfo_getfield("ftpNonProxyInfo");
    }
    
    private NonProxyInfo NonProxyInfo_httpNonProxyInfo() {
        return NonProxyInfo_getfield("httpNonProxyInfo");
    }
    
    private NonProxyInfo NonProxyInfo_getfield(final String fieldName) {
        try {
            final Class selectorClazz = parent.getClass();
            final ClassLoader cldr = selectorClazz.getClassLoader();
            final Class nonProxyInfoClazz = Class.forName(selectorClazz.getName() + "$"
                    + "NonProxyInfo", true, cldr);
            final Field f = nonProxyInfoClazz.getDeclaredField(fieldName);
            f.setAccessible(true);
            final Object realNPI = f.get(null);
            return new NonProxyInfo(realNPI);
        } catch (final Exception e) {
            throw new RuntimeException(e);
        }
    }
    
    private class NonProxyInfo {
        
        private final Object realNPI;
        
        private NonProxyInfo(final Object realNPI) {
            this.realNPI = realNPI;
        }
        
        public String hostsSource() {
            return getField("hostsSource", realNPI);
        }
        
        public RegexpPool hostsPool() {
            return getField("hostsPool", realNPI);
        }
        
        public void sethostsSource(final String val) {
            setField("hostsSource", realNPI, val);
        }
        
        public void sethostsPool(final RegexpPool val) {
            setField("hostsPool", realNPI, val);
        }
        
        public String property() {
            return getField("property", realNPI);
        }
        
        public String defaultVal() {
            return getField("defaultVal", realNPI);
        }
        
        public <T> T getField(final String fieldName, final Object o) {
            try {
                final Class clazz = realNPI.getClass();
                final Field f = clazz.getDeclaredField(fieldName);
                f.setAccessible(true);
                final Object res = f.get(o);
                return (T) res;
            } catch (final Exception e) {
                throw new RuntimeException(e);
            }
        }
        
        public void setField(final String fieldName, final Object o, final Object val) {
            try {
                final Class clazz = realNPI.getClass();
                final Field f = clazz.getDeclaredField(fieldName);
                f.setAccessible(true);
                f.set(o, val);
            } catch (final Exception e) {
                throw new RuntimeException(e);
            }
        }
    }
    
    @Override
    public List<Proxy> select(final URI paramURI) {
        if (paramURI == null) {
            throw new IllegalArgumentException("URI can't be null.");
        }
        final String str1 = paramURI.getScheme();
        Object localObject1 = paramURI.getHost();
        
        if (localObject1 == null) {
            
            Object localObject2 = paramURI.getAuthority();
            if (localObject2 != null) {
                int i = ((String) localObject2).indexOf('@');
                if (i >= 0) {
                    localObject2 = ((String) localObject2).substring(i + 1);
                }
                i = ((String) localObject2).lastIndexOf(':');
                if (i >= 0) {
                    localObject2 = ((String) localObject2).substring(0, i);
                }
                localObject1 = localObject2;
            }
        }
        
        if ((str1 == null) || (localObject1 == null)) {
            throw new IllegalArgumentException("protocol = " + str1 + " host = "
                    + (String) localObject1);
        }
        final Object localObject2 = new ArrayList(1);
        
        NonProxyInfo localNonProxyInfo1 = null;
        
        if ("http".equalsIgnoreCase(str1)) {
            localNonProxyInfo1 = NonProxyInfo_httpNonProxyInfo();
        } else if ("https".equalsIgnoreCase(str1)) {
            
            localNonProxyInfo1 = NonProxyInfo_httpNonProxyInfo();
        } else if ("ftp".equalsIgnoreCase(str1)) {
            localNonProxyInfo1 = NonProxyInfo_ftpNonProxyInfo();
        }
        
        final String str2 = str1;
        final NonProxyInfo localNonProxyInfo2 = localNonProxyInfo1;
        final String str3 = ((String) localObject1).toLowerCase();
        
        final Proxy localProxy = (Proxy) AccessController.doPrivileged(new PrivilegedAction() {
            
            @Override
            public Proxy run() {
                String str1 = null;
                int j = 0;
                InetSocketAddress localInetSocketAddress = null;
                
                int localObject1;
                
                for (int i = 0; i < DefaultProxySelector.props.length; i++) {
                    if (DefaultProxySelector.props[i][0].equalsIgnoreCase(str2)) {
                        for (localObject1 = 1; localObject1 < DefaultProxySelector.props[i].length; localObject1++) {
                            
                            str1 = NetProperties.get(DefaultProxySelector.props[i][localObject1]
                                    + "Host");
                            if ((str1 != null) && (str1.length() != 0)) {
                                break;
                            }
                        }
                        Object localObject3;
                        if ((str1 == null) || (str1.length() == 0)) {
                            
                            if (hasSystemProxies) {
                                String str333;
                                if (str2.equalsIgnoreCase("socket")) {
                                    str333 = "socks";
                                } else {
                                    str333 = str2;
                                }
                                localObject3 = DefaultProxySelector.this.getSystemProxy(str333,
                                    str3);
                                if (localObject3 != null) {
                                    return (Proxy) localObject3;
                                }
                            }
                            return Proxy.NO_PROXY;
                        }
                        
                        if (localNonProxyInfo2 != null) {
                            String str2 = NetProperties.get(localNonProxyInfo2.property());
                            synchronized (localNonProxyInfo2) {
                                if (str2 == null) {
                                    if (localNonProxyInfo2.defaultVal() != null) {
                                        str2 = localNonProxyInfo2.defaultVal();
                                    } else {
                                        localNonProxyInfo2.sethostsSource(null);
                                        localNonProxyInfo2.sethostsPool(null);
                                    }
                                } else if (str2.length() != 0) {
                                    
                                    str2 = str2 + "|localhost|127.*|[::1]|0.0.0.0|[::0]";
                                }
                                
                                if ((str2 != null)
                                        && (!str2.equals(localNonProxyInfo2.hostsSource()))) {
                                    localObject3 = new RegexpPool();
                                    final StringTokenizer localStringTokenizer = new StringTokenizer(
                                            str2, "|", false);
                                    try {
                                        while (localStringTokenizer.hasMoreTokens()) {
                                            ((RegexpPool) localObject3).add(localStringTokenizer
                                                    .nextToken().toLowerCase(), Boolean.TRUE);
                                        }
                                    } catch (final REException localREException) {
                                    }
                                    localNonProxyInfo2.sethostsPool(((RegexpPool) localObject3));
                                    localNonProxyInfo2.sethostsSource(str2);
                                }
                                
                                if ((localNonProxyInfo2.hostsPool() != null)
                                        && (localNonProxyInfo2.hostsPool().match(str3) != null)) {
                                    return Proxy.NO_PROXY;
                                }
                            }
                        }
                        
                        int localObject2;
                        
                        j = NetProperties.getInteger(
                            DefaultProxySelector.props[i][localObject1] + "Port", 0).intValue();
                        if ((j == 0) && (localObject1 < DefaultProxySelector.props[i].length - 1)) {
                            
                            for (localObject2 = 1; localObject2 < DefaultProxySelector.props[i].length - 1; localObject2++) {
                                if ((localObject2 != localObject1) && (j == 0)) {
                                    j = NetProperties.getInteger(
                                        DefaultProxySelector.props[i][localObject2] + "Port", 0)
                                            .intValue();
                                }
                            }
                        }
                        
                        if (j == 0) {
                            if (localObject1 == DefaultProxySelector.props[i].length - 1) {
                                j = DefaultProxySelector.this.defaultPort("socket");
                            } else {
                                j = DefaultProxySelector.this.defaultPort(str2);
                            }
                        }
                        
                        localInetSocketAddress = InetSocketAddress.createUnresolved(str1, j);
                        
                        if (localObject1 == DefaultProxySelector.props[i].length - 1) {
                            final int k = NetProperties.getInteger("socksProxyVersion", 5)
                                    .intValue();
                            return SocksProxy.create(localInetSocketAddress, k);
                        }
                        return new Proxy(Proxy.Type.HTTP, localInetSocketAddress);
                    }
                }
                
                return Proxy.NO_PROXY;
            }
        });
        ((List) localObject2).add(localProxy);
        
        return (List) localObject2;
    }
    
    @Override
    public void connectFailed(final URI paramURI, final SocketAddress paramSocketAddress,
            final IOException paramIOException) {
        if ((paramURI == null) || (paramSocketAddress == null) || (paramIOException == null)) {
            throw new IllegalArgumentException("Arguments can't be null.");
        }
    }
    
    private int defaultPort(final String paramString) {
        if ("http".equalsIgnoreCase(paramString)) {
            return 80;
        }
        if ("https".equalsIgnoreCase(paramString)) {
            return 443;
        }
        if ("ftp".equalsIgnoreCase(paramString)) {
            return 80;
        }
        if ("socket".equalsIgnoreCase(paramString)) {
            return 1080;
        }
        if ("gopher".equalsIgnoreCase(paramString)) {
            return 80;
        }
        return -1;
    }
    
    private final ProxySelector parent;
    
    public DefaultProxySelector(final ProxySelector parent) throws Exception {
        this.parent = parent;
        final Class clazz = parent.getClass();
        Field f = null;
        f = clazz.getDeclaredField("hasSystemProxies");
        f.setAccessible(true);
        hasSystemProxies = (Boolean) f.get(null);
        
    }
    
    private Proxy getSystemProxy(final String paramString1, final String paramString2) {
        try {
            final Class clazz = parent.getClass();
            Method m = null;
            m = clazz.getDeclaredMethod("getSystemProxy", String.class, String.class);
            m.setAccessible(true);
            final Object res = m.invoke(parent, paramString1, paramString2);
            return (Proxy) res;
        } catch (final Exception e) {
            throw new RuntimeException(e);
        }
    }
}
