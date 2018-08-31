package com.common.jsp.beans;

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.lang.reflect.Array;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.RandomAccess;

import com.google.common.reflect.TypeToken;

@SuppressWarnings("rawtypes")
public class BeanHusk {

	/**
	 * For {@code <jsp:useBean>}
	 */
	@Deprecated
	public BeanHusk() {
	}

	public BeanHusk(Object value) {
		setValue(value);
	}

	protected boolean valueSet;

	protected Object value; // bean or map or collection or array or primitive
	
	// parent bean property name or array index or Iterable offset or map entrySet offset
	// private String key;
	//private String displayName; // parent map key, otherwise same as key
	private TypeToken type; // resolved generic value type
	// combination of this bean properties and this array indexes or Iterable offsets
	private Map<String, BeanHusk> properties;

	@Override
	public String toString() {
		return super.toString();
	}

	public Object getValue() {
		// thisProperty.getReadMethod()
		return value;
	}

	public void setValue(Object value) {
		if (valueSet) {
			throw new IllegalStateException("value already set");
		}
		valueSet = true;
		this.value = value;
	}

	public BeanHusk getParent() {
		return null;
	}

	public String getDisplayName() {
		return null;
	}

	public TypeToken getType() {
		if (type == null) {
			if (value == null) {
				type = TypeToken.of(Object.class);
			} else {
				type = TypeToken.of(value.getClass());
			}
		}
		return type;
	}

	public String getTypeName() {
		String fullName = getType().toString();
		String simpleName = fullName.replaceAll("[a-zA-Z0-9.]*[.]", "");
		return simpleName;
	}

	public Map<String, BeanHusk> getProperties() {
		if (properties == null) {
			properties = new LinkedHashMap<>();
			if (value != null) {
				// bean properties first
				try {
					Class rawType = getType().getRawType();
					BeanInfo beanInfo = Introspector.getBeanInfo(value.getClass());
					for (PropertyDescriptor prop : beanInfo.getPropertyDescriptors()) {
						Method getter = prop.getReadMethod();
						if (getter != null && getter.getParameterTypes().length == 0) {
							String name = prop.getName();
							PropHusk propHusk = new PropHusk(this, prop);
							properties.put(name, propHusk);
						}
					}
				} catch (IntrospectionException e) {
					//
				}

				if (value.getClass().isArray()) {
				    int sz = Array.getLength(value);
				    for (int i = 0; i < sz; i++) {
				    	ArrayElement elem = new ArrayElement(this, i);
						properties.put(Integer.toString(i), elem);
				    }
				} else if (value instanceof List && value instanceof RandomAccess) {
				    List list = (List)value;
				    int sz = list.size();
				    for (int i = 0; i < sz; i++) {
				    	ListElement elem = new ListElement(this, i);
						properties.put(Integer.toString(i), elem);
				    }
				} else {
					if (value instanceof Map) {
						int i = 0;
						for (Entry<?,?> o : ((Map<?,?>)value).entrySet()) {
							MapElement elem = new MapElement(this, o);
							properties.put(Integer.toString(i), elem);
							i++;
						}
					} else if (value instanceof Iterable) {
						int i = 0;
						for (Object o : (Iterable)value) {
							IterableElement elem = new IterableElement(this, o);
							properties.put(Integer.toString(i), elem);
							i++;
						}
					}
				}
			}
		}
		return properties;
	}
	private static class ChildHusk extends BeanHusk {
		
		ChildHusk(BeanHusk parent, Object value) {
			super(value);
			this.parent = parent;
		}
		
		protected final BeanHusk parent;

		@Override
		public BeanHusk getParent() {
			return parent;
		}
		
	}

	private static class MapElement extends IterableElement {

		MapElement(BeanHusk parent, Map.Entry value) {
			super(parent, value);
		}
		
		@Override
		public String getDisplayName() {
			return getProperties().get("key").toString();
		}
	}

	private static class IterableElement extends ChildHusk {

		IterableElement(BeanHusk parent, Object value) {
			super(parent, value);
		}

	}

	private static class ListElement extends IndexedElement {

		ListElement(BeanHusk parent, int index) {
			super(parent, index, null);
		}
		
		@Override
		public Object getValue() {
			return ((List)parent).get(index);
		}
	}

	private static class ArrayElement extends IndexedElement {

		ArrayElement(BeanHusk parent, int index) {
			super(parent, index, null);
		}
		
		@Override
		public Object getValue() {
			return Array.get(parent, index);
		}
	}

	private static class IndexedElement extends ChildHusk {
		
		protected final int index;
		
		IndexedElement(BeanHusk parent, int index, Object value) {
			super(parent, value);
			this.index = index;
		}
		
		@Override
		public String getDisplayName() {
			return Integer.toString(index);
		}
	}

	private static class PropHusk extends ChildHusk {

		PropHusk(BeanHusk parent, PropertyDescriptor thisProperty) {
			super(parent, null);
			this.thisProperty = thisProperty;
		}

		private final PropertyDescriptor thisProperty;
		
		@Override
		public Object getValue() {
			try {
				return thisProperty.getReadMethod().invoke(parent);
			} catch (IllegalAccessException e) {
				throw new RuntimeException(e);
			} catch (InvocationTargetException e) {
				throw new RuntimeException(e);
			}
		}
	}

}
