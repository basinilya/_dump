package com.common.jsp.beans;

import java.util.ArrayList;
import java.util.Collection;

import junit.framework.TestCase;

public class TestClassComparator extends TestCase {

	private ClassComparator cc = new ClassComparator();

	@Override
	protected void setUp() throws Exception {
		super.setUp();
	}
	
	
	public void testA() throws Exception {
		aaa(0, null, null);

		aaa(0, Object.class, Object.class);
		aaa(0, Runnable.class, Runnable.class);
		aaa(0, int.class, int.class);

		aaa(-1, null, Object.class);
		aaa(-1, null, Runnable.class);
		aaa(-1, null, int.class);

		aaa(-1, Runnable.class, ArrayList.class);
		aaa(-1, int.class, AutoCloseable.class);
		aaa(-1, int.class, ArrayList.class);
		
		aaa(-1, Exception.class, RuntimeException.class);

		aaa(-1, Iterable.class, Collection.class);
		
		aaa(-1, javax.management.ValueExp.class, javax.naming.Name.class);

		aaa(-1, boolean.class, short.class);
	}
	
	private void aaa(int expected, Class<?> arg0, Class<?> arg1) throws Exception {
		assertEquals(expected, (int)Math.signum(cc.compare(arg0, arg1)));
		assertEquals(-expected, (int)Math.signum(cc.compare(arg1, arg0)));
	}
}
