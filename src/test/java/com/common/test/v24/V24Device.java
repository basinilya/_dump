
package com.common.test.v24;

import java.io.Serializable;

import javax.persistence.Embeddable;

/**
 * Represents a device on a serial bus. Has a 1:1 mapping with
 * {@link V24EndPoint}. Call
 * {@link V24EndPoint#createBeIdSg(int, int)} to get its beIdSg.
 *
 * @author basin
 */
@Embeddable
public class V24Device implements Serializable {

    private static final long serialVersionUID = 1L;

    private int v24DeviceId;

    /**
     * For XmlDecoder.
     *
     * @deprecated use {@link #V24Device(GtwayV24)}
     */
    @Deprecated
    public V24Device() {
        
    }

    public V24Device(GtwayV24 gtwayV24) {
        GtwayV24Data dataObj = gtwayV24.getF();
        v24DeviceId = dataObj.nextV24EndpointId();
    }

    /**
     * @return the device id part of beIdSg. The instanceId can be found in the parent bus.
     */
    public int getV24DeviceId() {
        return v24DeviceId;
    }

    /**
     * @param v24DeviceId the device id part of beIdSg
     * @deprecated for XMLDecoder
     */
    @Deprecated
    public void setV24DeviceId(int v24DeviceId) {
        this.v24DeviceId = v24DeviceId;
    }
}
