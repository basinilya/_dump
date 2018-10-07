package com.common.jsp.beans;

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.beans.PropertyEditor;
import java.beans.PropertyEditorManager;
import java.beans.PropertyEditorSupport;
import java.lang.reflect.Array;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.util.Collection;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.RandomAccess;

import org.apache.commons.beanutils.ConvertUtils;

import com.google.common.reflect.TypeToken;

@SuppressWarnings("rawtypes")
public class BeanHusk extends FactoryProvider {

	/**
	 * For {@code <jsp:useBean>}
	 */
	@Deprecated
	public BeanHusk() {
	}

	public BeanHusk(Object value) {
		this(true, value);
	}

	protected BeanHusk(boolean valueSet, Object value) {
		this.valueSet = valueSet;
		this.value = value;
	}
	
	@Override
	public String toString() {
		return "<root>";
	}

	private boolean failedGet;

	public boolean isFailedGet() {
		return failedGet;
	}

	private boolean valueSet;

	private Object value; // bean or map or collection or array or primitive
	
	// parent bean property name or array index or Iterable offset or map entrySet offset
	// private String key;
	//private String displayName; // parent map key, otherwise same as key
	protected TypeToken type; // resolved generic value type
	// combination of this bean properties and this array indexes or Iterable offsets
	private Map<String, BeanHusk> properties;

	public String getValueAsText() {
		String res = null;
		Object _value = getValue();
		if (_value != null) {
			PropertyEditor _editor = getPropertyEditor();
			_editor.setValue(_value);
			res = _editor.getAsText();
		}
		return res;
	}
	
	protected PropertyEditor editor;
	
	protected PropertyEditor getPropertyEditor() {
		if (editor == null) {
			Object _value = getValue();
			editor = PropertyEditorManager.findEditor( _value == null ? getTypeToken().getRawType() : _value.getClass() );
			if (editor == null) {
				editor = new PropertyEditorSupport() {

					@Override
					public String getAsText() {
						Object __value = getValue();
						return __value == null ? null : __value.toString(); //ConvertUtils.convert(getValue());
					}

					@Override
					public void setAsText(String text) throws IllegalArgumentException {
						Class rawType = getTypeToken().getRawType();
						Object __value = text;
						if (rawType != String.class) {
							__value = ConvertUtils.convert(text, rawType);
							if (__value instanceof String) {
								throw new UnsupportedOperationException("Can't convert");
							}
						}
						setValue(__value);
					}
				};
			}
		}
		return editor;
	}

	public final Object getValue() {
		if (!valueSet) {
			valueSet = true;
			failedGet = true;
			try {
				value = getValue0();
				failedGet = false;
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		return value;
	}

	protected Object getValue0() {
		throw new IllegalStateException("value not set yet");
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

	@Override
	public TypeToken getTypeToken() {
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
		String fullName = getTypeToken().toString();
		String simpleName = fullName.replaceAll("[a-zA-Z0-9.]*[.]", "")
				.replaceAll("capture#[0-9][0-9]*-of ", "")
				.replaceAll(" extends class ", " extends ");
		return simpleName;
	}

	public Integer getIndex() {
		return null;
	}

	public void setIndex(Integer index) {
		throw new UnsupportedOperationException();
	}

	public boolean isSetValueSupported() {
		return true;
	}
	
	public boolean isRemoveSupported() {
		return false;
	}
	
	public void remove() {
		throw new UnsupportedOperationException();
	}
	
	private static class NewListElement extends ListElement /* for getType & setIndex */ {

		NewListElement(BeanHusk parent, int defaultIndex) {
			super(parent, defaultIndex);
		}

		@Override
		public void setValue(Object value) {
			List ourList = (List)parent.getValue();
			ourList.add(index, value);
		}
		
		@Override
		public Object getValue0() {
			return null;
		}
		
		@Override
		public Integer getIndex() {
			return index;
		}

		@Override
		public boolean isRemoveSupported() {
			return false;
		}

		@Override
		public void setIndex(Integer index) {
			if (index < 0) {
				throw new IllegalArgumentException("negative index");
			}
			this.index = index;
		}

	}

	private static class NewArrayElement extends ArrayElement /* for getType & setIndex */ {

		NewArrayElement(BeanHusk parent, int defaultIndex) {
			super(parent, defaultIndex);
		}
		
		@Override
		public void setValue(Object value) {
			Object ourArray = parent.getValue();
			int sz = Array.getLength(ourArray);
			int newSz = Math.max(index, sz) + 1;
			Object newArray = Array.newInstance(getTypeToken().getRawType(), newSz);
			System.arraycopy(ourArray, 0, newArray, 0, Math.min(sz, index));
			Array.set(newArray, index, value);
			if (sz > index) {
				System.arraycopy(ourArray, index, newArray, index + 1, sz - index);
			}
			parent.setValue(newArray);
		}

		@Override
		protected Object getValue0() {
			return null;
		}
		
		@Override
		public Integer getIndex() {
			return index;
		}

		@Override
		public boolean isRemoveSupported() {
			return false;
		}

		@Override
		public void setIndex(Integer index) {
			if (index < 0) {
				throw new IllegalArgumentException("negative index");
			}
			this.index = index;
		}
	}

	private static class NewMapElement extends MapElement /* for getType & setIndex */ {

		NewMapElement(BeanHusk parent) {
			super(parent, null);
		}
		
		@Override
		public String getDisplayName() {
			return null;
		}

		@Override
		public void setValue(Object value) {
			Map.Entry entry = (Map.Entry)value;
			Map ourCol = (Map)parent.getValue();
			ourCol.put(entry.getKey(), entry.getValue());
		}
		
		@Override
		public boolean isSetValueSupported() {
			return true;
		}

		@Override
		public boolean isRemoveSupported() {
			return false;
		}
		
		@Override
		public Object getValue0() {
			return null;
		}

	}

	private static final String PROPNAME_NEW = "-1";

	private static class NewCollectionElement extends ChildHusk /* for getType & setIndex */ {

		NewCollectionElement(BeanHusk parent) {
			super(parent, true, null);
		}

		@Override
		public void setValue(Object value) {
			Collection ourCol = (Collection)parent.getValue();
			ourCol.add(value);
		}
		
		@Override
		public boolean isSetValueSupported() {
			return true;
		}
		
		@Override
		public Object getValue0() {
			return null;
		}
	}

	
	public Map<String, BeanHusk> getProperties() {
		if (properties == null) {
			properties = new LinkedHashMap<>();
			Object _value = getValue();
			if (_value != null) {

				if (_value.getClass().isArray()) {
				    int sz = Array.getLength(_value);
					if (isSetValueSupported()) {
						NewArrayElement elem = new NewArrayElement(this, sz);
						properties.put(PROPNAME_NEW, elem);
					}
				} else if (_value instanceof List) {
				    List list = (List)_value;
				    int sz = list.size();
				    NewListElement elem = new NewListElement(this, sz);
					properties.put(PROPNAME_NEW, elem);
				} else {
					if (_value instanceof Map) {
						NewMapElement elem = new NewMapElement(this);
						properties.put(PROPNAME_NEW, elem);
					} else if (_value instanceof Collection) {
						NewCollectionElement elem = new NewCollectionElement(this);
						properties.put(PROPNAME_NEW, elem);
					}
				}
				
				
				// bean properties first
				try {
					//Class rawType = getType().getRawType();
					BeanInfo beanInfo = Introspector.getBeanInfo(_value.getClass());
					PropertyDescriptor[] props = beanInfo.getPropertyDescriptors();
					for (int i = 0; i < 2; i++) {
						for (PropertyDescriptor prop : props) {
							Method getter = prop.getReadMethod();
							if (getter != null && getter.getParameterTypes().length == 0) {
								String name = prop.getName();
								if (i != 0 ^ "class".equals(name) ) {
									if (_value instanceof Map.Entry && "value".equals(name)) {
										// TODO: this setting remains in the global cache of Property Descriptors
										prop.setWriteMethod(Map.Entry.class.getMethod("setValue", Object.class) );
									}
									PropHusk propHusk = new PropHusk(this, prop);
									properties.put(name, propHusk);
								}
							}
						}
					}
				} catch (IntrospectionException | NoSuchMethodException e) {
					//
				}

				if (_value.getClass().isArray()) {
				    int sz = Array.getLength(_value);
				    for (int i = 0; i < sz; i++) {
				    	ArrayElement elem = new ArrayElement(this, i);
						properties.put(Integer.toString(i), elem);
				    }
				} else if (_value instanceof List && _value instanceof RandomAccess) {
				    List list = (List)_value;
				    int sz = list.size();
				    for (int i = 0; i < sz; i++) {
				    	ListElement elem = new ListElement(this, i);
						properties.put(Integer.toString(i), elem);
				    }
				} else {
					if (_value instanceof Map) {
						int i = 0;
						for (Entry<?,?> o : ((Map<?,?>)_value).entrySet()) {
							MapElement elem = new MapElement(this, o);
							properties.put(Integer.toString(i), elem);
							i++;
						}
					} else if (_value instanceof Iterable) {
						int i = 0;
						for (Object o : (Iterable)_value) {
							IterableElement elem = new IterableElement(this, true, o);
							properties.put(Integer.toString(i), elem);
							i++;
						}
					}
				}
			}
		}
		return properties;
	}

	private static class MapElement extends IterableElement {

		MapElement(BeanHusk parent, Map.Entry value) {
			super(parent, true, value);
		}
		
		@Override
		public String getDisplayName() {
			return getProperties().get("key").getValueAsText();
		}

		@Override
		public TypeToken getTypeToken() {
			if (type == null) {
				
				try {
					Method entrySet = parent.getTypeToken().getRawType().getMethod("entrySet");
					TypeToken entrySetType = parent.getTypeToken().resolveType(entrySet.getGenericReturnType());
					TypeToken resolvedIterableType = entrySetType.getSupertype(Iterable.class);
					Type valType = ((ParameterizedType)resolvedIterableType.getType()).getActualTypeArguments()[0];
					type = TypeToken.of(valType);

				} catch (NoSuchMethodException e) {
					throw new RuntimeException(e);
				}
			}
			return type;
		}

		@Override
		public void remove() {
			Object key = ((Map.Entry)getValue()).getKey();
			((Map)parent.getValue()).remove(key);
		}
	}

	private static class IterableElement extends ChildHusk {

		IterableElement(BeanHusk parent, boolean valueSet, Object value) {
			super(parent, valueSet, value);
		}
		
		@Override
		public void remove() {
			for (Entry<String, BeanHusk> x : parent.getProperties().entrySet()) {
				if (x.getValue() == this) {
					Iterator it = ((Iterable)parent.getValue()).iterator();
					for (int index = Integer.parseInt(x.getKey());index >= 0;index--) {
						it.next();
					}
					it.remove();
					break;
				}
			}
		}

		@Override
		public boolean isRemoveSupported() {
			return true;
		}
	}

	private static class ListElement extends IndexedElement {

		ListElement(BeanHusk parent, int index) {
			super(parent, index, false, null);
		}
		
		@Override
		public Object getValue0() {
			return ((List)parent.getValue()).get(index);
		}
		
		@Override
		public void setValue(Object value) {
			((List)parent.getValue()).set(index, value);
		}
		
		@Override
		public void remove() {
			((List)parent.getValue()).remove(index);
		}
	}

	private static class ArrayElement extends IndexedElement {

		ArrayElement(BeanHusk parent, int index) {
			super(parent, index, false, null);
		}
		
		@Override
		protected Object getValue0() {
			return Array.get(parent.getValue(), index);
		}

		@Override
		public boolean isRemoveSupported() {
			return parent.isSetValueSupported();
		}
		
		@Override
		public void setValue(Object value) {
			Object ourArray = parent.getValue();
			Array.set(ourArray, index, value);
			if (parent.isSetValueSupported()) {
				parent.setValue(ourArray);
			}
		}

		@Override
		public void remove() {
			Object ourArray = parent.getValue();
			int sz = Array.getLength(ourArray);
			int newSz = sz - 1;
			Object newArray = Array.newInstance(getTypeToken().getRawType(), newSz);
			System.arraycopy(ourArray, 0, newArray, 0, Math.min(newSz, index));
			if (newSz > index) {
				System.arraycopy(ourArray, index + 1, newArray, index, newSz - index);
			}
			parent.setValue(newArray);
		}

		@Override
		public TypeToken getTypeToken() {
			if (type == null) {
				type = parent.getTypeToken().getComponentType();
			}
			return type;
		}

	}

	private static class IndexedElement extends IterableElement {
		
		protected int index;

		IndexedElement(BeanHusk parent, int index, boolean valueSet, Object value) {
			super(parent, valueSet, value);
			this.index = index;
		}
		
		@Override
		public String getDisplayName() {
			return Integer.toString(index);
		}
		
		@Override
		public boolean isSetValueSupported() {
			return true;
		}
	}

	private static class PropHusk extends ChildHusk {

		PropHusk(BeanHusk parent, PropertyDescriptor thisProperty) {
			super(parent, false, null);
			this.thisProperty = thisProperty;
		}

		private final PropertyDescriptor thisProperty;
		
		@Override
		public String getDisplayName() {
			return thisProperty.getName();
		}

		@Override
		public boolean isSetValueSupported() {
			return thisProperty.getWriteMethod() != null;
		}

		@Override
		public void setValue(Object value) {
			try {
				thisProperty.getWriteMethod().invoke(parent.getValue(), value);
			} catch (IllegalAccessException e) {
				throw new RuntimeException(e);
			} catch (InvocationTargetException e) {
				throw new RuntimeException(e);
			}
		}

		@Override
		protected PropertyEditor getPropertyEditor() {
			if (editor == null) {
				editor = thisProperty.createPropertyEditor(getValue());
			}
			return super.getPropertyEditor();
		}
		
		@Override
		public Object getValue0() {
			try {
				return thisProperty.getReadMethod().invoke(parent.getValue());
			} catch (IllegalAccessException e) {
				throw new RuntimeException(e);
			} catch (InvocationTargetException e) {
				throw new RuntimeException(e);
			}
		}

		@Override
		public TypeToken getTypeToken() {
			if (type == null) {
				Type memberType = thisProperty.getReadMethod().getGenericReturnType();
				TypeToken tt = parent.getTypeToken();
				if (!tt.isPrimitive()) {
					Object parentVal = parent.getValue();
					if (parentVal != null) {
						tt = tt.getSubtype(parentVal.getClass());
					}
				}
				type = tt.resolveType(memberType);
			}
			return type;
		}
	}

	private static abstract class ChildHusk extends BeanHusk {
		
		ChildHusk(BeanHusk parent, boolean valueSet, Object value) {
			super(valueSet, value);
			this.parent = parent;
		}
		
		protected final BeanHusk parent;

		@Override
		public BeanHusk getParent() {
			return parent;
		}
		
		@Override
		public void setValue(Object value) {
			throw new UnsupportedOperationException("Not implemented yet");
		}
		
		@Override
		public boolean isSetValueSupported() {
			return false;
		}

		@Override
		public String toString() {
			try {
				return parent.toString() + "." + getDisplayName();
			} catch (Exception e) {
				return "<error>";
			}
		}

		@Override
		public TypeToken getTypeToken() {
			if (type == null) {
				TypeToken resolvedIterableType = parent.getTypeToken().getSupertype(Iterable.class);
				Type valType = ((ParameterizedType)resolvedIterableType.getType()).getActualTypeArguments()[0];
				type = TypeToken.of(valType);
			}
			return type;
		}

	
	}
}
