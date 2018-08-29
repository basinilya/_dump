package com.common.test.v24;

import java.io.Serializable;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.Map;
import javax.persistence.Embeddable;

/**
 * Represents the Ethernet Serial Adapter, usually Moxa NPort.
 */
@Embeddable
public class EtherSerialAdapter implements Serializable {

    private static final long serialVersionUID = 1L;

    public static final String MAKE_MOXA = "moxa";

    public static final boolean DEF_HIDDEN = false;

    private String name;

    private String hostName;

    private String mac;

    private String make;

    private String model;

    private Integer numSerial;

    private Boolean hidden;

    private Map<Integer, EtherSerialPort> serialsByTcpServerPort = new HashMap<>();

    public Map<Integer, EtherSerialPort> getSerialsByTcpServerPort() {
        return serialsByTcpServerPort;
    }

    public void setSerialsByTcpServerPort(Map<Integer, EtherSerialPort> serialsByTcpServerPort) {
        this.serialsByTcpServerPort = serialsByTcpServerPort;
    }

    /**
     * @return number of serial ports
     */
    public Integer getNumSerial() {
        return numSerial;
    }

    public void setNumSerial(Integer numSerial) {
        this.numSerial = numSerial;
    }

    /**
     * Do not show this auto-discovered adapter in Configurator and do not try to connect to its ports.
     *
     * @return the non-nullable value
     */
    // !!! do not change return type to not confuse XMLEncoder
    public Boolean isHidden() {
        return hidden == null ? DEF_HIDDEN : hidden;
    }

    /**
     * @return The nullable value for XMLEncoder
     * @deprecated Use {@link #isHidden()} instead
     */
    @Deprecated
    public Boolean getHidden() {
        return hidden;
    }

    public void setHidden(Boolean hidden) {
        this.hidden = hidden;
    }

    /**
     * @return the convenience name
     */
    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    /**
     * @return host name or IP address
     */
    public String getHostName() {
        return hostName;
    }

    public void setHostName(String hostName) {
        this.hostName = hostName;
    }

    /**
     * @return the MAC address as a HEX string
     */
    public String getMac() {
        return mac;
    }

    /**
     * @param mac the MAC address as a HEX string
     * @throws IllegalArgumentException if invalid MAC
     */
    public void setMac(String mac) throws IllegalArgumentException {
        if (mac != null) {
            int len = mac.length();
            CHARS12OK:
            if (len != 0) {
                if (len == 12) {
                    mac = mac.toUpperCase();
                    char c;
                    do {
                        if ((--len) < 0) {
                            break CHARS12OK;
                        }
                        c = mac.charAt(len);
                    } while (c >= '0' && c <= '9' || c >= 'A' && c <= 'F');
                }
                throw new IllegalArgumentException("MAC address invalid: " + mac);
            }
        }
        this.mac = mac;
    }

    /**
     * @return the device maker
     */
    public String getMake() {
        return make;
    }

    public void setMake(String make) {
        this.make = make;
    }

    /**
     * @return the device model
     */
    public String getModel() {
        return model;
    }

    public void setModel(String model) {
        this.model = model;
    }

    /**
     * @param adaptersByIpStr the map returned by {@link GtwayV24Data#getAdaptersByIpStr()}
     * @param ipStr the IP address to associate with this adapter
     * @throws IllegalArgumentException if bad or duplicate IP address
     */
    public void putToMap(Map<String, EtherSerialAdapter> adaptersByIpStr, String ipStr) throws IllegalArgumentException {
        try {
            InetAddress addrObj = parseIpAddr(ipStr);
            ipStr = addrObj.getHostAddress(); // legally throw NPE
            EtherSerialAdapter existing = adaptersByIpStr.put(ipStr, this);
            if (existing != null && existing != this) {
                adaptersByIpStr.put(ipStr, existing);
                throw new IllegalArgumentException("duplicate IP address " + ipStr
                        + "; remove existing adapter " + existing.getName() + "before adding this one");
            }
        } catch (UnknownHostException ex) {
            throw new IllegalArgumentException(ex);
        }
    }

    /**
     * Guaranteed offline IP addr parse.
     *
     * @param hostArg valid IP address string
     * @return the parsed address
     * @throws UnknownHostException if bad IP address
     */
    // Copy of InetAddress.getAllByName 
    public static InetAddress parseIpAddr(final String hostArg)
            throws UnknownHostException {
        String host = hostArg;
        if (host == null || host.length() == 0) {
            return null;
        }

        boolean ipv6Expected = false;
        if (host.charAt(0) == '[') {
            // This is supposed to be an IPv6 literal
            if (host.length() > 2 && host.charAt(host.length() - 1) == ']') {
                host = host.substring(1, host.length() - 1);
                ipv6Expected = true;
            } else {
                // This was supposed to be a IPv6 address, but it's not!
                throw new UnknownHostException(host + ": invalid IPv6 address");
            }
        }

        // if host is an IP address, we won't do further lookup
        if (Character.digit(host.charAt(0), 16) != -1
                || (host.charAt(0) == ':')) {
            return InetAddress.getByName(host); // By now it's hopefully offline
        } else if (ipv6Expected) {
            // We were expecting an IPv6 Litteral, but got something else
            throw new UnknownHostException("[" + host + "]");
        }
        throw new UnknownHostException("host name and not an IP address: " + hostArg);
    }

}
