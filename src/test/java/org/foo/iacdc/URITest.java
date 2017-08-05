package org.foo.iacdc;

import java.net.URI;
import java.net.URISyntaxException;

public class URITest {
    
    public static void main(final String[] args) throws Exception {
        final URI parentURI = new URI("ftp://hostname/aaa/bbb/f");
        final URI maybeChildURI = new URI("fTp://hOstname:21/aaa/bbb/f");
        
        if ("".length() == 1) {
            System.out.println(parentURI.getScheme());
            System.out.println(parentURI.getUserInfo());
            System.out.println(parentURI.getHost());
            System.out.println(parentURI.getPort());
            System.out.println(parentURI.getPath());
            System.out.println(parentURI.getQuery());
            System.out.println(parentURI.getFragment());
            parentURI.equals(maybeChildURI);
        }
        
        // System.out.println(uri0.resolve(uri10));
        // System.out.println(uri10.resolve(uri0));
        // new URL("ftp://hostname/bbb/ccc/ddd").sameFile(null);
        System.out.println("isParentOf: '" + isParentOf(parentURI, maybeChildURI, 21) + "'");
        
    }
    
    private static boolean isParentOf(final URI parentURI, final URI maybeChildURI,
            final int defaultPort) {
        // Compare the protocols.
        if (!((parentURI.getScheme() == maybeChildURI.getScheme()) || (parentURI.getScheme() != null && parentURI
                .getScheme().equalsIgnoreCase(maybeChildURI.getScheme())))) {
            return false;
        }
        
        // Compare the ports.
        int port1, port2;
        port1 = (parentURI.getPort() != -1) ? parentURI.getPort() : defaultPort;
        port2 = (maybeChildURI.getPort() != -1) ? maybeChildURI.getPort() : defaultPort;
        if (port1 != port2) {
            return false;
        }
        
        // Compare the hosts.
        final String h1 = parentURI.getHost();
        final String h2 = maybeChildURI.getHost();
        if (!(h1 == h2 || (h1 != null && h1.equalsIgnoreCase(h2)))) {
            return false;
        }
        
        try {
            final URI parentPath = new URI(parentURI.getPath());
            final URI maybeChildPath = new URI(maybeChildURI.getPath());
            
            return !parentPath.relativize(maybeChildPath).getPath().startsWith("/");
            
        } catch (final URISyntaxException e) {
        }
        
        return false;
    }
    
}
