package com.common.jsp.beans;

import com.common.test.v24.GtwayV24;
import com.common.test.v24.GtwayV24Data;

import junit.framework.TestCase;

public class TestAddDefaultImpl
    extends TestCase
{
    public void testAddDefaultImpl()
        throws Exception
    {
        Editor editor = new Editor();
        GtwayV24 gtway = new GtwayV24();
        GtwayV24Data rootPojo = gtway.getF();
        editor.setRootObject( rootPojo );
    }
}
