package com.common.jsp.beans;

import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Collection;
import java.util.EnumMap;
import java.util.List;
import java.util.Map;

import com.google.common.reflect.TypeToken;

import junit.framework.TestCase;

@SuppressWarnings("serial")
public class TestConstructor extends TestCase {

	protected void tearDown() throws Exception {
		FactoryProvider.testFilter = null;
		super.tearDown();
	};
	
	public void testA() throws Exception {
		// FactoryProvider.testFilter = s -> EnumMap.class.getName().equals(s.getName());
		FactoryProvider.testFilter = s -> ArrayList.class.getName().equals(s.getName());
		
		//Map<? extends Enum<?>, ?> m = null;
		//m = new EnumMap<>(RetentionPolicy.class);
		
		//Enum<?> en = null;
		//new EnumMap<Enum<?>, Object>();
		
		final TypeToken<?> tt = new TypeToken<List<? extends Number>>() {
		};
		FactoryProvider factoryProvider = new FactoryProvider() {
			
			@Override
			public TypeToken getTypeToken() {
				return tt;
			}
		};
		
		
		TypeToken<?> expected1 = new TypeToken<java.util.ArrayList<java.lang.Number>>() {};
		TypeToken<?> expected2 = new TypeToken<java.util.Collection<? extends java.lang.Number>>() {};
		Factory factory = factoryProvider.getFactories().get(null).get(0);
		System.out.println(factory);
		assertEquals(expected1, factory.getContext());
		assertEquals(expected2, factory.getParamsProviders().get(0).getTypeToken());
// new java.util.EnumMap<? extends java.lang.Enum<?>, ?>(java.util.EnumMap<capture#1-of ? extends java.lang.Enum<?>&java.lang.Enum<K>, ? extends capture#2-of ? extends class java.lang.Object>)
// new java.util.EnumMap<java.lang.Enum<K>, java.lang.Object>(java.util.EnumMap<java.lang.Enum<K>, ?>)

	}

	class MyList<T extends Throwable> extends ArrayList<T> {
		public MyList(Collection<? extends T> col) {
			super(col);
		}
	}
}
