package com.common.test.v24;

import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;
import javax.persistence.Embeddable;
import javax.persistence.Embedded;

/**
 * Holds all {@link com.common.test.v24.GtwayV24} properties that
 * have no explicit mapping in DBMapperLib. Stored in DB as XML.
 *
 * @author basin
 */
@Embeddable
public class GtwayV24Data implements Serializable {

    private static final long serialVersionUID = 1L;

    @Embedded
    private NPortSettings nportSettings = new NPortSettings();

    private int v24EndpointSeq;

    private Map<String, EtherSerialAdapter> adaptersByIpStr = new HashMap<>();

    /**
     * Use {@link EtherSerialAdapter#putToMap(Map,String)} to put elements into this map.
     *
     * @return the map of adapters by IP address
     */
    public Map<String, EtherSerialAdapter> getAdaptersByIpStr() {
        return adaptersByIpStr;
    }

    public void setAdaptersByIpStr(Map<String, EtherSerialAdapter> adaptersById) {
        this.adaptersByIpStr = adaptersById;
    }

    public int nextV24EndpointId() {
        return ++v24EndpointSeq;
    }
    
    public int getV24EndpointSeq() {
        return v24EndpointSeq;
    }

    public void setV24EndpointSeq(int v24EndpointSeq) {
        this.v24EndpointSeq = v24EndpointSeq;
    }

    /**
     * @return the common NPort settings
     */
    public NPortSettings getNportSettings() {
        return nportSettings;
    }

    public void setNportSettings(NPortSettings nportSettings) {
        this.nportSettings = nportSettings;
    }
}
