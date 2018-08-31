package com.common.jsp.beans;

import java.util.LinkedHashMap;
import java.util.Map;

import com.google.common.reflect.TypeToken;

@SuppressWarnings("rawtypes")
public class BeanHusk {
	private Object value; // bean or map or collection or array or primitive
	private BeanHusk parent;
	// parent bean property name or array index or Iterable offset or map entrySet offset
	// private String key;
	private String displayName; // parent map key, otherwise same as key
	private TypeToken type; // resolved generic value type
	// combination of this bean properties and this array indexes or Iterable offsets
	private Map<String, BeanHusk> properties;

	@Override
	public String toString() {
		return super.toString();
	}

	public Object getValue() {
		return value;
	}

	public void setValue(Object value) {
		this.value = value;
	}

	public BeanHusk getParent() {
		return parent;
	}

	public void setParent(BeanHusk parent) {
		this.parent = parent;
	}

	public String getDisplayName() {
		return displayName;
	}

	public void setDisplayName(String displayName) {
		this.displayName = displayName;
	}

	public TypeToken getType() {
		if (type == null) {
			if (parent == null) {
				if (value == null) {
					type = TypeToken.of(Object.class);
				} else {
					type = TypeToken.of(value.getClass());
				}
			} else {
				throw new UnsupportedOperationException("not yet");
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
		}
		return properties;
	}
}
