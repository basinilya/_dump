package com.common.jsp.beans;

import java.util.Comparator;

/**
 * primitive < interface < class; superclass < class
 * 
 */
public class ClassComparator implements Comparator<Class<?>> {
	
	public static final ClassComparator INSTANCE = new ClassComparator(); 
	
	@Override
	public int compare(Class<?> o1, Class<?> o2) {
		if (o1 == o2) {
			return 0;
		}
		if (o1 == null) {
			return -1;
		}
		if (o2 == null) {
			return 1;
		}

		int firstWeight = o1.isPrimitive() ? 0 : (o1.isInterface() ? 1 : 2);
		int secondWeight = o2.isPrimitive() ? 0 : (o2.isInterface() ? 1 : 2);

		if (firstWeight < secondWeight) {
			return -1;
		}

		if (firstWeight > secondWeight) {
			return 1;
		}

		switch (firstWeight) {
		case 0:
			return compareByName(o1, o2);
		case 1:
			return compareNumbersThenNames(getInterfaceCount(o1), getInterfaceCount(o2), o1, o2);
		default:
			return compareNumbersThenNames(getAncestorCount(o1), getAncestorCount(o2), o1, o2);
		}
	}

	private int compareNumbersThenNames(int f, int s, Class<?> first, Class<?> second) {
		if (f > s) {
			return 1;
		} else if (f < s) {
			return -1;
		} else {
			return compareByName(first, second);
		}
	}

	private int getAncestorCount(Class<?> objectClass) {
		int res = 0;
		for (Class<?> i = objectClass; i != null; i = i.getSuperclass()) {
			res++;
		}
		return res;
	}

	private int getInterfaceCount(Class<?> interfaceClass) {
		int res = 1;
		for (Class<?> s : interfaceClass.getInterfaces()) {
			res += getInterfaceCount(s);
		}
		return res;
	}

	private int compareByName(Class<?> a, Class<?> b) {
		int res = a.getName().compareTo(b.getName());
		// we do not support different class loaders
		return res;
	}
}