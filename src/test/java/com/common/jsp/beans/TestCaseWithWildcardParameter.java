package com.common.jsp.beans;

import java.util.Date;
import java.util.Map;

import junit.framework.TestCase;

public abstract class TestCaseWithWildcardParameter<T extends Date> extends TestCase
{
    public Map<Integer, T> getIncomplete()
    {
        return null;
    }
}
