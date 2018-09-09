package com.common.jsp.beans;

import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.lang.reflect.Type;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.EnumMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import junit.framework.TestCase;

import com.google.common.reflect.ClassPath;
import com.google.common.reflect.ClassPath.ClassInfo;
import com.google.common.reflect.TypeToken;

@SuppressWarnings({ "serial", "rawtypes", "unchecked" })
public class TestTypeUtils extends TestCase {
	public static class MyCollection1 extends ArrayList<Integer> {
	}

	public static class MyCollection2 extends ArrayList<Timestamp> {
	}

	public static class MyCollection3<T> extends ArrayList<T> {
	}

	public static class MyCollection4 extends ArrayList {
	}

	public static Collection<Timestamp> fld;

	@Override
	protected void setUp() throws Exception {
		super.setUp();

		Field getter = TestTypeUtils.class.getField("fld");
		Type memberType = getter.getGenericType();

		resolved = TypeToken.of(TestTypeUtils.class).resolveType(memberType);
		System.out.println("type of fld: " + resolved);

	}

	private TypeToken<?> resolved;

	public void testAssignable() throws Exception {
		// bad: fld = new MyCollection1();
		fld = new MyCollection2();
		fld = new MyCollection3(); // just a warning
		fld = new MyCollection4(); // just a warning

		assertTrue(checkAssignable(resolved, ArrayList.class));
		assertFalse(checkAssignable(resolved, MyCollection1.class));
		assertTrue(checkAssignable(resolved, MyCollection2.class));
		assertTrue(checkAssignable(resolved, MyCollection3.class));
		assertTrue(checkAssignable(resolved, MyCollection4.class));
	}

	public void testScanner() throws Exception {
		ClassPath classPath = ClassPath.from(Thread.currentThread().getContextClassLoader());
		String prefix = TestTypeUtils.class.getName() + '$';

		Set<TypeToken<?>> expected = new HashSet<>(Arrays.asList(TypeToken.of(MyCollection2.class),
				new TypeToken<MyCollection3<Timestamp>>() {
				}, TypeToken.of(MyCollection4.class)));
		Set<TypeToken<?>> actual = TypeUtils.findImplementations(classPath, prefix, resolved, null);
		assertEquals(expected, actual);
	}

	public void testWild() throws Exception {
		EnumMap<RetentionPolicy, Object> xx = new EnumMap<>(RetentionPolicy.class);

		Map<?, ?> m1 = xx;
		m1.getClass();

		Map<? extends Enum<?>, ?> m2 = xx;
		m2.getClass();

		Map<Enum<?>, ?> m3 = null;

		// Type mismatch: cannot convert from EnumMap<RetentionPolicy,Object> to Map<Enum<?>,?>
		// m3 = xx;
		
		String.valueOf(m3);

		Map<String, ?> m4 = null;
		// Type mismatch: cannot convert from EnumMap<RetentionPolicy,Object> to Map<String,?>
		// m4 = xx;

		String.valueOf(m4);
		
		Map<? extends Throwable, ?> m5 = null;
		// Type mismatch: cannot convert from EnumMap<RetentionPolicy,Object> to Map<? extends Throwable,?>
		// m5 = xx;

		String.valueOf(m5);

		TypeToken<?> tt1 = new TypeToken<Map<?, ?>>() {
		};
		TypeToken<?> tt2 = new TypeToken<Map<? extends Enum<?>, ?>>() {
		};
		TypeToken<?> tt3 = new TypeToken<Map<Enum<?>, ?>>() {
		};
		TypeToken<?> tt4 = new TypeToken<Map<String, ?>>() {
		};
		TypeToken<?> tt5 = new TypeToken<Map<? extends Throwable, ?>>() {
		};

		assertNotNull(TypeUtils.getUncheckedSubtype(tt1, EnumMap.class));
		assertNotNull(TypeUtils.getUncheckedSubtype(tt2, EnumMap.class));
		assertNull(TypeUtils.getUncheckedSubtype(tt5, EnumMap.class));
		assertNull(TypeUtils.getUncheckedSubtype(tt3, EnumMap.class));
		assertNull(TypeUtils.getUncheckedSubtype(tt4, EnumMap.class));
	}

	private static boolean checkAssignable(TypeToken<?> resolved, Class<?> implClass) {
		boolean res = TypeUtils.getUncheckedSubtype(resolved, implClass) != null;
		System.out.println("fld = new " + implClass.getSimpleName() + "() -> " + (res ? "yes" : "no"));
		return res;
	}

}