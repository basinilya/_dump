package com.common.jsp.beans;

import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.util.Map;

import junit.framework.TestCase;

public class TestNewParameterizedType extends TestCase {

	public void testA() throws Exception {
		Type t = TypeUtils.newParameterizedType(Map.class, new Type[] { String.class, Integer.class });
		ParameterizedType pt = (ParameterizedType)t;
		Type[] targs = pt.getActualTypeArguments();
		assertEquals(String.class, targs[0]);
		assertEquals(Integer.class, targs[1]);
	}


}
