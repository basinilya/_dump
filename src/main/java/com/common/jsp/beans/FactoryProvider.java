package com.common.jsp.beans;

import java.beans.PropertyEditor;
import java.beans.PropertyEditorManager;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.Modifier;
import java.lang.reflect.Type;
import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Scanner;
import java.util.Set;

import com.common.test.v24.V24ProtocolEm;
import com.google.common.reflect.ClassPath;
import com.google.common.reflect.TypeToken;

public abstract class FactoryProvider {
	
	private static final HashMap<Map.Entry<TypeToken, Class<?>> ,Set<Class<?>>> AAA = new HashMap<>();

	private final MyLazyMap myLazyMap = new MyLazyMap();
	private final Map<String, List<Factory>> factories = Collections.unmodifiableMap(myLazyMap);
	
	public abstract TypeToken getType();
	
	public Map<String, List<Factory>> getFactories() {
		return factories;
	}
	
	private class MyLazyMap extends HashMap<String, List<Factory>> {
		@Override
		public List<Factory> get(Object key) {
			if (key == null) {
				key = "";
			}
			List<Factory> res = super.get(key);
			if (res == null && key instanceof String) {
				String restrictString = (String)key;
				res = new ArrayList<>();
				try {
					TypeToken tt = getType();
					Class<?> propClazz = tt.getRawType();
					ClassLoader loader = Thread.currentThread().getContextClassLoader();
					// TODO: restrictString can contain number of elements and array class
					Class<?> restrictClazz = 
							restrictString.isEmpty()
							? (propClazz.isPrimitive() ? propClazz : Object.class)
							: Class.forName(restrictString, true, loader );
					if (restrictClazz.isAssignableFrom(propClazz)) {
						restrictClazz = propClazz;
					}
					if (propClazz.isAssignableFrom(restrictClazz)) {
						Map.Entry<TypeToken, Class<?>> ck = new AbstractMap.SimpleEntry<>(tt, restrictClazz);
						Set<Class<?>> candidates = AAA.get(ck);
						boolean found = candidates != null;
						if (!found) {
							if (Modifier.isFinal( restrictClazz.getModifiers() )) {
								candidates = new HashSet<>();
							} else {
						        ClassPath classPath = ClassPath.from( TypeUtils.mkScannableClassLoader ( loader )  );
						        candidates = TypeUtils.findImplementations(classPath, "", tt, restrictClazz);
							}
							candidates.add(restrictClazz);
						}
						for (Iterator<Class<?>> it = candidates.iterator();it.hasNext();) {
							boolean good = false;
							Class<?> candidate = it.next();
							PropertyEditor editor = PropertyEditorManager.findEditor(candidate);
							if (editor != null) {
								good = true;
								// TODO: this is thread-insafe instance of PE
								res.add(new EditorBasedFactory(editor));
							}
							for (Constructor<?> cons : candidate.getConstructors()) {
								if (Modifier.isPublic(cons.getModifiers())) {
									good = true;
									res.add(new ConstructorFactory(cons));
								}
							}
							if (!good) {
								it.remove();
							}
						}
						if (!found) {
							AAA.put(ck, candidates);
						}
					}
				} catch (ClassNotFoundException e) {
					// ignore
				} catch (IOException e) {
					// ignore
				}
				put(restrictString, res);
			}
			return res;
		}
	}

	private static class EditorBasedFactory extends Factory {
		private final PropertyEditor editor;

		EditorBasedFactory(PropertyEditor editor) {
			this.editor = editor;
		}
		
		@Override
		public Object getInstance(Object[] params) throws Exception {
			editor.setAsText((String)params[0]);
			return editor.getValue();
		}
		
		@Override
		public List<FactoryProvider> getParamsProviders() {
			List<FactoryProvider> res = new ArrayList<>();
			res.add(new SompleFactoryProvider(TypeToken.of( String.class)));
			return res;
		}

		@Override
		public String[] getTags() {
			return editor.getTags();
		}
	}
	
	private static class SompleFactoryProvider extends FactoryProvider {

		private final TypeToken tt;

		SompleFactoryProvider (TypeToken tt) {
			this.tt = tt;
		}

		@Override
		public TypeToken getType() {
			return tt;
		}
	}

	private class ConstructorFactory extends Factory {
		private final Constructor<?> cons;

		ConstructorFactory(Constructor<?> cons) {
			if (cons == null) {
				throw new NullPointerException();
			}
			this.cons = cons;
		}
		
		@Override
		public Object getInstance(Object[] params) throws Exception {
			return cons.newInstance(params);
		}
		
		private TypeToken getContext() {
			TypeToken context = getType();
			context = context.getSubtype(cons.getDeclaringClass());
			return context;
		}

		@Override
		public List<FactoryProvider> getParamsProviders() {
			List<FactoryProvider> res = new ArrayList<>();
			TypeToken context = getContext();
			for (Type paramType : cons.getGenericParameterTypes()) {
				TypeToken resolved = context.resolveType(paramType);
				res.add(new SompleFactoryProvider(resolved));
			}
			return res;
		}
		
		@Override
		public String toString() {
			StringBuilder sb = new StringBuilder("new ");
			// String name = cons.getDeclaringClass().getName();
			String name;
			try {
				name = getContext().toString();
				//Type context = getContext().getType();
				//if (!(context instanceof Class)) {
				//	name = context.toString();
				//}
			} catch (Exception e) {
				name = "<error>";
			}
			
			sb.append(name).append('(');
			
			try {
				String comma = "";
				for (FactoryProvider x : getParamsProviders()) {
					sb.append(comma).append(x.getType().toString());
					comma = ", ";
				}
			} catch (Exception e) {
				sb.append("<error>");
			}

			sb.append(')');
			return sb.toString();
		}
	}
	
	public static void main(String[] args) {
		//PropertyEditor editor = PropertyEditorManager.findEditor(byte.class);
		//System.out.println(editor);
		//System.out.println(Arrays.asList(editor.getTags()));
	}
}
