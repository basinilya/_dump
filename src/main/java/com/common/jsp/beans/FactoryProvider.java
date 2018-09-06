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
import com.google.common.base.Objects;
import com.google.common.reflect.ClassPath;
import com.google.common.reflect.TypeToken;

public abstract class FactoryProvider {
	
	private static final HashMap<Map.Entry<TypeToken, Class<?>> ,Set<Class<?>>> CACHE = new HashMap<>();

	private final MyLazyMap myLazyMap = new MyLazyMap();
	private final Map<String, List<Factory>> factories = Collections.unmodifiableMap(myLazyMap);
	
	public abstract TypeToken getType();
	
	public Map<String, List<Factory>> getFactories() {
		System.out.println("    " + this);
		return factories;
	}
	
	protected Factory getParentFactory() {
		return null;
	}
	
	protected boolean aaa() {
		return false;
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
						Set<Class<?>> candidates = CACHE.get(ck);
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
								// TODO: this is thread-unsafe instance of PE
								tryAdd(res, new EditorBasedFactory(candidate, editor));
							}
							for (Constructor<?> cons : candidate.getConstructors()) {
								if (Modifier.isPublic(cons.getModifiers())) {
									good = true;
									tryAdd(res, new ConstructorFactory(cons));
								}
							}
							if (!good) {
								it.remove();
							}
						}
						if (!found) {
							CACHE.put(ck, candidates);
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

	private void tryAdd(final List<Factory> res, final Factory factory) {
		FactoryProvider x = this;
		Factory pf;
		while ((pf = x.getParentFactory()) != null) {
			if (factory.equals(pf)) {
				return;
			}
			x = pf.getProvider();
		}
		res.add(factory);
	}

	private class EditorBasedFactory extends Factory {
		private final Class<?> resultType;
		private final PropertyEditor editor;

		@Override
		FactoryProvider getProvider() {
			return FactoryProvider.this;
		}

		@Override
		public boolean equals(Object obj) {
			if (this == obj)
				return true;
			if (obj != null && this.getClass() == obj.getClass()) {
				EditorBasedFactory other = (EditorBasedFactory)obj;
				return Objects.equal(this.resultType, other.resultType);
			}
			return false;
		};
		

		@Override
		public int hashCode() {
			final int prime = 31;
			int result = 1;
			result = prime * result + ((resultType == null) ? 0 : resultType.hashCode());
			return result;
		}

		EditorBasedFactory(Class<?> resultType, PropertyEditor editor) {
			this.resultType = resultType;
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
			res.add(new SimpleFactoryProvider(this, TypeToken.of( String.class)));
			return res;
		}

		@Override
		public String toString() {
			return "from string...";
		}

		@Override
		public String[] getTags() {
			return editor.getTags();
		}
	}
	
	private static class SimpleFactoryProvider extends FactoryProvider {

		private final Factory parentFactory;

		private final TypeToken tt;

		SimpleFactoryProvider (Factory parentFactory, TypeToken tt) {
			this.parentFactory = parentFactory;
			this.tt = tt;
		}
		
		@Override
		protected Factory getParentFactory() {
			return parentFactory;
		}

		@Override
		public TypeToken getType() {
			return tt;
		}
	}

	private class ConstructorFactory extends Factory {
		private final Constructor<?> cons;

		@Override
		FactoryProvider getProvider() {
			return FactoryProvider.this;
		}

		ConstructorFactory(Constructor<?> cons) {
			if (cons == null) {
				throw new NullPointerException();
			}
			this.cons = cons;
		}
		
		@Override
		public int hashCode() {
			final int prime = 31;
			int result = 1;
			result = prime * result + ((cons == null) ? 0 : cons.hashCode());
			return result;
		}

		@Override
		public boolean equals(Object obj) {
			if (obj != null && this.getClass() == obj.getClass()) {
				ConstructorFactory other = (ConstructorFactory)obj;
				return Objects.equal(this.cons, other.cons);
			}
			return false;
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
				res.add(new SimpleFactoryProvider(this, resolved));
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
