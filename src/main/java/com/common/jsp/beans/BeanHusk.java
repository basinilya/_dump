package com.common.jsp.beans;

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
	private String typeName; // resolved generic value type
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
		return type;
	}

	public void setType(TypeToken type) {
		this.type = type;
	}

	public String getTypeName() {
		return typeName;
	}

	public void setTypeName(String typeName) {
		this.typeName = typeName;
	}

	public Map<String, BeanHusk> getProperties() {
		return properties;
	}

	public void setProperties(Map<String, BeanHusk> properties) {
		this.properties = properties;
	}
}
