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
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.common.test.v24.V24ProtocolEm;
import com.google.common.reflect.ClassPath;
import com.google.common.reflect.TypeToken;

public abstract class FactoryProvider {
	
	//private static final MyLazyMap myLazyMap = new MyLazyMap();

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
				String clazzName = (String)key;
				res = new ArrayList<>();
				try {
					TypeToken tt = getType();
					Class<?> propClazz = tt.getRawType();
					ClassLoader loader = Thread.currentThread().getContextClassLoader();
					Class<?> restrictClazz = 
							clazzName.isEmpty()
							? (propClazz.isPrimitive() ? propClazz : Object.class)
							: Class.forName(clazzName, true, loader );
					if (restrictClazz.isAssignableFrom(propClazz)) {
						restrictClazz = propClazz;
					}
					if (propClazz.isAssignableFrom(restrictClazz)) {
						Set<Class<?>> candidates;
						if (Modifier.isFinal( restrictClazz.getModifiers() )) {
							candidates = new HashSet<>();
						} else {
					        ClassPath classPath = ClassPath.from( TypeUtils.mkScannableClassLoader ( loader )  );
					        candidates = TypeUtils.findImplementations(classPath, "", tt, restrictClazz);
						}
						candidates.add(restrictClazz);
						for (Class<?> candidate : candidates) {
							PropertyEditor editor = PropertyEditorManager.findEditor(candidate);
							if (editor != null) {
								// TODO: this is thread-insafe instance of PE
								res.add(new EditorBasedFactory(editor));
							}
							for (Constructor<?> cons : candidate.getConstructors()) {
								if (cons.isAccessible()) {
									res.add(new ConstructorFactory(cons));
								}
							}
						}
						// restrictClazz.get
					}
				} catch (ClassNotFoundException e) {
					// ignore
				} catch (IOException e) {
					// ignore
				}
				put(clazzName, res);
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
			this.cons = cons;
		}
		
		@Override
		public Object getInstance(Object[] params) throws Exception {
			return cons.newInstance(params);
		}
		
		@Override
		public List<FactoryProvider> getParamsProviders() {
			List<FactoryProvider> res = new ArrayList<>();
			TypeToken context = getType();
			context = context.getSubtype(cons.getDeclaringClass());
			for (Type paramType : cons.getGenericParameterTypes()) {
				TypeToken resolved = context.resolveType(paramType);
				res.add(new SompleFactoryProvider(resolved));
			}
			return res;
		}
	}
	
	public static void main(String[] args) {
		//PropertyEditor editor = PropertyEditorManager.findEditor(byte.class);
		//System.out.println(editor);
		//System.out.println(Arrays.asList(editor.getTags()));
	}
}
