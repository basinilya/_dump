package com.common.test.v24;

import java.io.Serializable;

import javax.persistence.Transient;

public class GtwayV24 implements Serializable {

    private static final long serialVersionUID = 1L;

    private GtwayV24Data f = new GtwayV24Data();


    /**
     * @return the actual data POJO
     */
    @Transient
    public GtwayV24Data getF() {
        return f;
    }

    public void setF(GtwayV24Data f) {
        this.f = f;
    }

}
