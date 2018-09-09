package com.google.common.reflect;

import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;

import org.checkerframework.checker.nullness.qual.Nullable;

public class GuavaReflectAccessHelper {
	
	public static ParameterizedType newParameterizedTypeWithOwner(
		      @Nullable Type ownerType, Class<?> rawType, Type... arguments) {
		return Types.newParameterizedTypeWithOwner(ownerType, rawType, arguments);
	}

	public static ParameterizedType newParameterizedType(Class<?> rawType, Type... arguments) {
		return Types.newParameterizedType(rawType, arguments);
	}

}
