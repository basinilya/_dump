package com.common.test.v24;

import javax.persistence.Embeddable;

/**
 * Represents the serial bus with ESPA devices.
 */
@Embeddable
public class EspaBus extends SerialBus<V24Device> {

    private static final long serialVersionUID = 1L;

    public static final char DEF_OUR_ADDRESS = '2';

    private Integer pollInterval;

    private Character ourAddress;

    @Override
    public V24Device putDefaultDevice(GtwayV24 config, String busAddr) {
        V24Device v24dev = new V24Device(config);
        getEndpointsByBusAddr().put(busAddr, v24dev);
        return v24dev;
    }

    @Override
    public V24ProtocolEm getV24Protocol() {
        return V24ProtocolEm.ESPA;
    }

    /**
     * @return nullable our ESPA address
     * @deprecated use {@link #getOurAddress_()}
     */
    @Deprecated
    public Character getOurAddress() {
        return ourAddress;
    }

    public char getOurAddress_() {
        return ourAddress == null ? DEF_OUR_ADDRESS : ourAddress;
    }

    public void setOurAddress(Character ourAddress) {
        this.ourAddress = ourAddress;
    }

    /**
     * @return if a positive number, then we're the control station and we poll with this interval.
     */
    public Integer getPollInterval() {
        return pollInterval;
    }

    public void setPollInterval(Integer pollInterval) {
        this.pollInterval = pollInterval;
    }

}
