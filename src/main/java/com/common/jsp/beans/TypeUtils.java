package com.common.jsp.beans;

import java.lang.reflect.GenericArrayType;
import java.lang.reflect.GenericDeclaration;
import java.lang.reflect.Modifier;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.lang.reflect.TypeVariable;
import java.lang.reflect.WildcardType;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.HashSet;
import java.util.Objects;
import java.util.Set;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.common.collect.FluentIterable;
import com.google.common.reflect.ClassPath;
import com.google.common.reflect.ClassPath.ResourceInfo;
import com.google.common.reflect.TypeToken;
import com.google.common.reflect.ClassPath.ClassInfo;

public class TypeUtils {
	public static TypeToken<?> getUncheckedSubtype(TypeToken<?> resolved, Class<?> implClass) {
		try {
			TypeToken<?> res = resolved.getSubtype(implClass);
			
			TypeVariable<?>[] tvars = implClass.getTypeParameters();
			Type resType = res.getType();
			if (resType instanceof ParameterizedType) {
				ParameterizedType pt = (ParameterizedType)resType;
				Type[] targs = pt.getActualTypeArguments();
				for (int i = 0; i < tvars.length; i++) {
					TypeVariable<?> tvar = tvars[i];
					TypeToken<?> tvarTt = TypeToken.of(tvar);
					if (targs[i] instanceof WildcardType) {
						WildcardType targ = (WildcardType)targs[i];
						Type[] ubounds = targ.getUpperBounds();
						for (int j = 0; j < ubounds.length; j++) {
							Type ubound = ubounds[j];
							boolean b3 = tvarTt.isSubtypeOf(ubound);
							if (!b3) {
								TypeToken<?> targTt = TypeToken.of(targ);
								for (Type tvarBound : tvar.getBounds()) {
									if (!targTt.isSubtypeOf(tvarBound)) {
										return null;
									}
								}
							}
						}
					} else {
						if (!(tvar.getBounds().length == 1 && tvar.getBounds()[0] == Object.class)) {
							Type targ = targs[i];
							boolean b5 = tvarTt.isSupertypeOf(targ);
							if (!b5) {
								return null;
							}
						}
					}
				}
			}
			return res;
		} catch (IllegalArgumentException ex) {
			// ;
			return isAnySupertypeAssignable(resolved, implClass) ? TypeToken.of(implClass) : null;
		}
	}

	public static boolean isAnySupertypeAssignable(TypeToken<?> resolved, Class<?> implClass) {
		return TypeToken.of(implClass).getTypes().stream()
				.anyMatch(supertype -> null != getUncheckedRawSubtype(resolved, supertype));
	}

	public static TypeToken<?> getUncheckedRawSubtype(TypeToken<?> resolved, TypeToken<?> implTypeToken) {
		if (implTypeToken.getType() instanceof ParameterizedType) {
			return null; // this prevents assignments of Collection<Integer> to Collection<Timestamp>
		}
		try {
			return resolved.getSubtype(implTypeToken.getRawType());
		} catch (IllegalArgumentException ex) {
			return null;
		}
	}

	public static Stream<ClassInfo> getAllClasses(ClassPath classPath) {
		return classPath.getResources().stream().filter(s -> s instanceof ClassInfo).map(s -> (ClassInfo)s);
	}

	public static Predicate<ClassInfo> byPrefixFilter(String prefix) {
		return s -> s.getName().startsWith(prefix);
	}

	public static final Function<? super ClassInfo, ? extends Class<?>> LOADCLASS_MAPPER = s -> {
		try {
			return s.load();
		} catch (Throwable e) {
			return null;
		}
	};

	public static Function<? super Class<?>, ? extends TypeToken<?>> subtypeMapper(TypeToken<?> resolved) {
		return candidate -> TypeUtils.getUncheckedSubtype(resolved, candidate);
	};

	public static Predicate<? super Class<?>> bySuperFilter(TypeToken<?> resolved, Class<?> restrict) {
		Class<?> clazz = restrict == null ? resolved.getRawType() : restrict;
		return candidate ->
		candidate != null && ((clazz.isAssignableFrom(candidate) && Modifier.isPublic(candidate.getModifiers())
					&& (Modifier.isStatic(candidate.getModifiers()) || candidate.getEnclosingClass() == null)
					&& !Modifier.isAbstract(candidate.getModifiers())
					&& !Modifier.isInterface(candidate.getModifiers()))
					|| candidate.isPrimitive());
	}

	public static Set<TypeToken<?>> findImplementations(ClassPath classPath, Predicate<ClassInfo> classInfoFilter, TypeToken<?> resolved,
			Class<?> restrict) {
		Stream<ClassInfo> classInfos = getAllClasses(classPath);
		if (classInfoFilter != null) {
			classInfos = classInfos.filter(classInfoFilter);
		}
		return classInfos
		.map(LOADCLASS_MAPPER)
		.filter(bySuperFilter(resolved, restrict))
		.map(subtypeMapper(resolved))
		.filter(Objects::nonNull)
		.collect(Collectors.toSet())
		;
	}

	public static Set<TypeToken<?>> findImplementations2(ClassPath classPath, String prefix, TypeToken<?> resolved,
			Class<?> restrict) {
		Class<?> clazz = restrict == null ? resolved.getRawType() : restrict;
		Set<TypeToken<?>> actual = new HashSet<>();

		for (ClassInfo classInfo : classPath.getAllClasses()) {
			if (!classInfo.getName().startsWith(prefix)) {
				continue;
			}
			final Class<?> candidate;
			try {
				candidate = classInfo.load();
			} catch (Throwable e) {
				continue;
			}
			if (clazz.isAssignableFrom(candidate) && Modifier.isPublic(candidate.getModifiers())
					&& (Modifier.isStatic(candidate.getModifiers()) || candidate.getEnclosingClass() == null)
					&& !Modifier.isAbstract(candidate.getModifiers())
					&& !Modifier.isInterface(candidate.getModifiers())) {
				TypeToken<?> tt = TypeUtils.getUncheckedSubtype(resolved, candidate);
				if (tt != null) {
					actual.add(tt);
				}
			}
		}
		return actual;
	}

	public static ClassLoader mkScannableClassLoader(ClassLoader parent) {
		if (Object.class.getClassLoader() == null) {
			try {
				URL url = Object.class.getResource("Object.class");
				String s = url.toString().replaceFirst("^jar:(.*)![^!]*$", "$1");
				url = new URL(s);
				URLClassLoader ucl = new URLClassLoader(new URL[] { url }, parent);
				return ucl;
			} catch (Throwable e) {
				//
			}
		}
		return parent;
	}
}
