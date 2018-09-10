package com.common.jsp.beans;

import java.beans.PropertyEditor;
import java.beans.PropertyEditorManager;
import java.beans.PropertyEditorSupport;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.Modifier;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.lang.reflect.TypeVariable;
import java.lang.reflect.WildcardType;
import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Scanner;
import java.util.Set;
import java.util.TreeSet;
import java.util.function.Predicate;

import org.apache.commons.beanutils.ConvertUtils;

import com.common.test.v24.V24ProtocolEm;
import com.google.common.reflect.ClassPath;
import com.google.common.reflect.TypeToken;
import com.google.common.reflect.GuavaReflectAccessHelper;
import com.google.common.reflect.ClassPath.ClassInfo;

public abstract class FactoryProvider {
	
	private static final Map<Map.Entry<TypeToken, Class<?>> ,Set<TypeToken<?>>> CACHE = Collections.synchronizedMap(new HashMap<>());

	private final MyLazyMap myLazyMap = new MyLazyMap();
	private final Map<String, List<Factory>> factories = Collections.unmodifiableMap(myLazyMap);
	
	public abstract TypeToken getTypeToken();
	
	public Map<String, List<Factory>> getFactories() {
		return factories;
	}
	
	protected Factory getParentFactory() {
		return null;
	}

	private class MyLazyMap extends HashMap<String, List<Factory>> {
		@Override
		public List<Factory> get(Object key) {
			if (key == null) {
				key = "";
			}
			List<Factory> res1 = super.get(key);
			if (res1 == null && key instanceof String) {
				String restrictString = (String)key;
				TreeSet<Factory> res = new TreeSet<>();
				try {
					TypeToken tt = getTypeToken();
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
						Set<TypeToken<?>> candidates = CACHE.get(ck);
						boolean found = candidates != null;
						if (!found) {
							if (Modifier.isFinal( restrictClazz.getModifiers() )) {
								candidates = new HashSet<>();
							} else {
								ClassPath classPath = ClassPath.from(TypeUtils.mkScannableClassLoader(loader));
								candidates = TypeUtils.findImplementations(classPath, testFilter, tt, restrictClazz);
							}
							// TODO: maybe Stream.concat
							if (TypeUtils.bySuperFilter(tt, restrictClazz).test(restrictClazz)) {
								TypeToken<?> restrictTt = TypeUtils.getUncheckedSubtype(tt, restrictClazz);
								if (restrictTt != null) {
									candidates.add(restrictTt);
								}
							}
						}
						for (Iterator<TypeToken<?>> it = candidates.iterator();it.hasNext();) {
							boolean good = false;
							TypeToken<?> candidate = it.next();
							Type ct = candidate.getType();
							if (ct instanceof ParameterizedType) {
								ParameterizedType pt = (ParameterizedType)ct;
								Class<?> ptClazz = ((Class<?>)pt.getRawType());
								TypeVariable<?>[] tvars = ptClazz.getTypeParameters();
								Type[] targs = pt.getActualTypeArguments();
								boolean doIt = false;
								for (int i = 0; i < targs.length; i++) {
									if (targs[i] instanceof WildcardType) {
										WildcardType wt = (WildcardType)targs[i];
										Type[] wtUbounds = wt.getUpperBounds();
										Type[] tvarBounds = tvars[i].getBounds();
										if (wtUbounds.length == 1 && tvarBounds.length == 1) {
											TypeToken<?> ubound1 = TypeToken.of(wtUbounds[0]);
											TypeToken<?> ubound2 = TypeToken.of(tvarBounds[0]);
											if (ubound1.isSubtypeOf(ubound2)) {
												doIt = true;
												targs[i] = ubound1.getType();
											} else if (ubound2.isSubtypeOf(ubound1)) {
												doIt = true;
												targs[i] = ubound2.getType();
											}
										}
									}
								}
								if (doIt) {
									pt = GuavaReflectAccessHelper.newParameterizedTypeWithOwner(pt.getOwnerType(), ptClazz, targs);
									candidate = TypeToken.of(pt);
								}
							}
							Class<?> candidateClazz = candidate.getRawType();
							PropertyEditor editor = PropertyEditorManager.findEditor(candidateClazz);
							if (editor == null && (candidateClazz == Character.class || candidateClazz == char.class)) {
								editor = new PropertyEditorSupport() {
									@Override
									public String getAsText() {
										Object __value = getValue();
										return __value == null ? null : __value.toString();
									}

									@Override
									public void setAsText(String text) throws IllegalArgumentException {
										Objects.requireNonNull(text);
										if (text.length() != 1) {
											throw new IllegalArgumentException("length must be 1");
										}
										setValue(text.charAt(0));
									}
								};
							}
							if (editor != null) {
								good = true;
								// TODO: this is thread-unsafe instance of PE
								tryAdd(res, new EditorBasedFactory(candidateClazz, editor));
							}
							Constructor<?>[] constructors = candidateClazz.getConstructors();
							for (int i = 0; i < constructors.length; i++) {
								Constructor<?> cons = constructors[i];
								if (Modifier.isPublic(cons.getModifiers())) {
									good = true;
									tryAdd(res, new ConstructorFactory(candidate, cons, i));
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
				res1 = new ArrayList<>(res);
				put(restrictString,  res1);
			}
			return res1;
		}
	}

	private void tryAdd(final Collection<Factory> res, final Factory factory) {
		FactoryProvider x = this;
		Factory pf;
		while ((pf = x.getParentFactory()) != null) {
			if (factory.equals(pf)) {
				return;
			}
			x = pf.getProvider();
		}
		// factory.getParamsProviders().get(0).getFactories()
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
				return Objects.equals(this.resultType, other.resultType);
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
		public TypeToken getTypeToken() {
			return tt;
		}
	}

	private class ConstructorFactory extends EditorBasedFactory {
		private final TypeToken<?> context;
		private final Constructor<?> cons;
		private final int iCons;

		ConstructorFactory(TypeToken<?> context, Constructor<?> cons, int iCons) {
			super(null, null);
			if (context == null || cons == null) {
				throw new NullPointerException();
			}
			this.context = context;
			this.cons = cons;
			this.iCons = iCons;
		}
		
		@Override
		public int hashCode() {
			final int prime = 31;
			int result = 1;
			result = prime * result + ((cons == null) ? 0 : cons.hashCode());
			result = prime * result + ((context == null) ? 0 : context.hashCode());
			result = prime * result + iCons;
			return result;
		}

		@Override
		public boolean equals(Object obj) {
			if (this == obj)
				return true;
			if (getClass() != obj.getClass())
				return false;
			ConstructorFactory other = (ConstructorFactory) obj;
			if (cons == null) {
				if (other.cons != null)
					return false;
			} else if (!cons.equals(other.cons))
				return false;
			if (context == null) {
				if (other.context != null)
					return false;
			} else if (!context.getRawType().equals(other.context.getRawType()))
				return false;
			if (iCons != other.iCons)
				return false;
			return true;
		}

		@Override
		public Object getInstance(Object[] params) throws Exception {
			return cons.newInstance(params);
		}
		
		TypeToken getContext() {
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
					sb.append(comma).append(x.getTypeToken().toString());
					comma = ", ";
				}
			} catch (Exception e) {
				sb.append("<error>");
			}

			sb.append(')');
			return sb.toString();
		}

		Class<?> getRawType() {
			return getContext().getRawType();
		}
		
		@Override
		public String[] getTags() {
			return null;
		}
		
		@Override
		public int compareTo(Factory o) {
			if (this.equals(o)) {
				return 0;
			}
			int res = super.compareTo(o);
			if (res != 0) {
				return res;
			}
			
			ConstructorFactory other = (ConstructorFactory)o;
			
			Class<?> thisRawType = this.getRawType();
			Class<?> otherRawType = other.getRawType();

			res = JavaSEProfile.WHITEBLACK_COMPARATOR.compare(thisRawType, otherRawType);
			if (res != 0) {
				return res;
			}
			if (this.iCons > other.iCons) {
				return 1;
			}
			if (this.iCons < other.iCons) {
				return -1;
			}
			return 0;
		}

	}
	
	static Predicate<ClassInfo> testFilter = null;
}
