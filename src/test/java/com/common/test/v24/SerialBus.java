package com.common.test.v24;

import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;

import javax.persistence.Embeddable;

/**
 * Represents an abstract serial bus. Allows to replug the whole bus with serial devices to another adapter. Do not set
 * serial port properties if you want to keep the current serial port settings.
 *
 * @author basin
 * @param <D> device class
 */
@Embeddable
public abstract class SerialBus<D extends V24Device> implements Serializable {

    private static final long serialVersionUID = 1L;

    public static final int DATA_BITS_5 = 0;
    public static final int DATA_BITS_6 = 1;
    public static final int DATA_BITS_7 = 2;
    public static final int DATA_BITS_8 = 3;

    public static final int PARITY_NONE = 0;
    public static final int PARITY_ODD = 1;
    public static final int PARITY_EVEN = 2;
    public static final int PARITY_MARK = 3;
    public static final int PARITY_SPACE = 4;

    public static final int STOP_BITS_1 = 0;
    public static final int STOP_BITS_1_5 = 1;
    public static final int STOP_BITS_2 = 2;

    public static final int DTR_FLOW_ENABLE = 16;
    public static final int DSR_FLOW_ENABLE = 32;
    public static final int DTRDSR_FLOW_ENABLE = 48;

    public static final int RTS_FLOW_ENABLE = 1;
    public static final int CTS_FLOW_ENABLE = 2;
    public static final int RTSCTS_FLOW_ENABLE = 3;

    public static final int XON_FLOW_ENABLE = 4;
    public static final int XOFF_FLOW_ENABLE = 8;
    public static final int XONXOFF_FLOW_ENABLE = 12;

    private Integer baudRate;
    private Integer dataBits;
    private Integer parity;
    private Integer stopBits;

    private Integer RtsCts;
    private Integer XonXoff;
    private Integer DtrDsr;

    private Map<String, ? extends V24Device> endpointsByBusAddr = new HashMap<>(); // EclipseLink NPE when <String,D>

    /**
     * @return endpoints by their bus address
     */
    public Map<String, D> getEndpointsByBusAddr() {
        return (Map<String, D>) endpointsByBusAddr;
    }

    public void setEndpointsByBusAddr(Map<String, D> endpointsByBusAddr) {
        this.endpointsByBusAddr = endpointsByBusAddr;
    }

    public abstract D putDefaultDevice(GtwayV24 config, String busAddr);

    public abstract V24ProtocolEm getV24Protocol();

    /**
     * Actual baud rate. For example, set to 9600 for 9600bps. The available range is dependent on NPort.
     *
     * @return the baudRate
     */
    public Integer getBaudRate() {
        return baudRate;
    }

    public void setBaudRate(Integer baudRate) {
        this.baudRate = baudRate;
    }

    /**
     * Available constants: DATA_BITS_5, DATA_BITS_6, DATA_BITS_7, DATA_BITS_8.
     *
     * @return the dataBits mode
     */
    public Integer getDataBits() {
        return dataBits;
    }

    public void setDataBits(Integer dataBits) {
        this.dataBits = dataBits;
    }

    /**
     * Available constants: PARITY_NONE, PARITY_ODD, PARITY_EVEN, PARITY_MARK, PARITY_SPACE.
     *
     * @return the parity mode
     */
    public Integer getParity() {
        return parity;
    }

    public void setParity(Integer parity) {
        this.parity = parity;
    }

    /**
     * Available constants: STOP_BITS_1, STOP_BITS_1_5, STOP_BITS_2.
     *
     * @return the stopBits mode
     */
    public Integer getStopBits() {
        return stopBits;
    }

    public void setStopBits(Integer stopBits) {
        this.stopBits = stopBits;
    }

    /**
     * Available ORed constants RTS_FLOW_ENABLE, CTS_FLOW_ENABLE, RTSCTS_FLOW_ENABLE. If you want to disable this flow
     * control, you can set it to zero. The default value is disabled(zero).
     *
     * @return the RtsCts mode
     */
    public Integer getRtsCts() {
        return RtsCts;
    }

    public void setRtsCts(Integer RtsCts) {
        this.RtsCts = RtsCts;
    }

    /**
     * Available ORed constants XON_FLOW_ENABLE, XOFF_FLOW_ENABLE, XONXOFF_FLOW_ENABLE. If you want to disable this flow
     * control, you can set it to zero. The default value is disabled(zero).
     *
     * @return the XonXoff mode
     */
    public Integer getXonXoff() {
        return XonXoff;
    }

    public void setXonXoff(Integer XonXoff) {
        this.XonXoff = XonXoff;
    }

    /**
     * Available ORed constants DTR_FLOW_ENABLE, DSR_FLOW_ENABLE, DTRDSR_FLOW_ENABLE. If you want to disable this flow
     * control, you can set it to zero. The default value is disabled(zero).
     *
     * @return the DtrDsr mode
     */
    public Integer getDtrDsr() {
        return DtrDsr;
    }

    public void setDtrDsr(Integer DtrDsr) {
        this.DtrDsr = DtrDsr;
    }

}
