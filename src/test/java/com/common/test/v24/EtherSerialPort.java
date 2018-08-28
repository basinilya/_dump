package com.common.test.v24;

import java.io.Serializable;
import javax.persistence.Embeddable;

/**
 * Represents one port of a Serial Ethernet Adapter. Do not set nportCommandPort if you want to keep the current serial
 * port settings.
 */
@Embeddable
public class EtherSerialPort implements Serializable {

    private static final long serialVersionUID = 1L;

    public static final int BASE_DATA = 950;

    public static final int BASE_CMD = 966;

    /**
     * COM1...COM16: 950...965:966...981
     * <p>
     * COM17...COM31: 982...997:998...1013
     *
     * @param base {@link #BASE_DATA} or {@link #BASE_CMD}
     * @param PortIndex one-based COM port index
     * @return tcp port
     */
    public static int calcPort(int base, int PortIndex) {
        return base + ((PortIndex - 1) / 16 * 32) + PortIndex - 1;
    }

    // default non-polling ESPA should be harmless
    private SerialBus<?> bus = new EspaBus();

    /**
     * @return Application protocol config or null if port disabled
     */
    public SerialBus<?> getBus() {
        return bus;
    }

    public void setBus(SerialBus<?> bus) {
        this.bus = bus;
    }

    private Integer nportCommandPort;

    /**
     * @return The command port for NPort Real Com mode or Tcp Server mode.
     */
    public Integer getNportCommandPort() {
        return nportCommandPort;
    }

    public void setNportCommandPort(Integer nportCommandPort) {
        this.nportCommandPort = nportCommandPort;
    }
}
