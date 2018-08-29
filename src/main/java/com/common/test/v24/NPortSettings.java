package com.common.test.v24;

import java.io.Serializable;
import javax.persistence.Embeddable;

/**
 * Global settings for Moxa NPort devices.
 */
@Embeddable
public class NPortSettings implements Serializable {
    
    private static final long serialVersionUID = 1L;

    public static final boolean DEF_DISCOVERY = true;

    private Boolean discovery;

    /**
     * Auto discover NPort devices on local subnet.
     *
     * @return the non-nullable value
     */
    // !!! do not change return type to not confuse XMLEncoder
    public Boolean isDiscovery() {
        return discovery == null ? DEF_DISCOVERY : discovery;
    }

    /**
     * @return The nullable value for XMLEncoder
     * @deprecated Use {@link #isDiscovery()} instead
     */
    @Deprecated
    public Boolean getDiscovery() {
        return discovery;
    }

    public void setDiscovery(Boolean discovery) {
        this.discovery = discovery;
    }
}
