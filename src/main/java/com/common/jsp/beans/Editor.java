package com.common.jsp.beans;

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.beans.PropertyEditor;
import java.beans.PropertyEditorManager;
import java.lang.reflect.Array;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.logging.Level;
import java.util.logging.Logger;

public class Editor
{
	private Object rootObject;

	private ArrayList<String> pathEntryNames = new ArrayList<>();
	private ArrayList<Object> pathEntries = new ArrayList<>();

	public ArrayList<Object> getPathEntries()
	{
		return pathEntries;
	}

	public void setPathEntries( ArrayList<Object> pathEntries )
	{
		this.pathEntries = pathEntries;
	}

	public ArrayList<String> getPathEntryNames()
	{
		return pathEntryNames;
	}

	public void setPathEntryNames( ArrayList<String> pathEntryNames )
	{
		this.pathEntryNames = pathEntryNames;
	}

	public Object getRootObject()
	{
		return rootObject;
	}

	public void setRootObject( Object rootPojo )
	{
		LOGGER.log( Level.INFO, "> setRootObject {0}" , new Object[] { rootPojo } );
		this.rootObject = rootPojo;
		pathEntryNames.clear();
		pathEntries.clear();
	}

	public Object getLeafObject() {
		return pathEntries.isEmpty( ) ? rootObject : pathEntries.get( pathEntries.size(  ) - 1 );
	}

	public static class PropertyRow {
	    private String nameAsText;
	    private String index;
	    private String type;
		private String valueAsText;

	    public PropertyRow(String nameAsText, String index, String type,
				String valueAsText) {
			super();
			this.nameAsText = nameAsText;
			this.index = index;
			this.type = type;
			this.valueAsText = valueAsText;
		}

	    public String getNameAsText() {
			return nameAsText;
		}
		public void setNameAsText(String nameAsText) {
			this.nameAsText = nameAsText;
		}
		public String getIndex() {
			return index;
		}
		public void setIndex(String index) {
			this.index = index;
		}
		public String getType() {
			return type;
		}
		public void setType(String type) {
			this.type = type;
		}
		public String getValueAsText() {
			return valueAsText;
		}
		public void setValueAsText(String valueAsText) {
			this.valueAsText = valueAsText;
		}
	}

	public String getLeafObjectAsText() {
		Object leafObject = getLeafObject();
		return toString(leafObject);
	}
	
	private String toString(Object leafObject) {
	    String res = null;
		if (leafObject != null) {
			PropertyEditor editor = PropertyEditorManager.findEditor( leafObject.getClass() );
			if (editor != null) {
				editor.setValue( leafObject );
				res = editor.getAsText( );
			}
			if (res == null) {
				res = leafObject.toString( );
			}
	    }
		return res;
	}

	public static int toPropertyIndex(String propertyName) {
		int n = -1;
		try {
			n = Integer.parseInt( propertyName );
		} catch (NumberFormatException e) {
			//
		}
		return n;
	}

	public void addPathEntry( String propertyName )
	{
		LOGGER.log( Level.INFO, "> addPathEntry {0}" , new Object[] { propertyName } );
		//if (rootObject == null) {
		//	throw new IllegalStateException("root object not set");
		//}
		pathEntryNames.add( propertyName );
		Object newLeaf = null;
		Object leafObject = getLeafObject();
		Class<?> propertyType = null;
		if (leafObject != null) {
			if (leafObject.getClass().isArray()) {
				propertyType = leafObject.getClass().getComponentType();
				int i = toPropertyIndex( propertyName );
				if (i > -1) {
					try {
						newLeaf = Array.get( leafObject, i );
					} catch (ArrayIndexOutOfBoundsException e) {
						//
					}
				}
			} else if (leafObject instanceof List) {
				int i = toPropertyIndex( propertyName );
				try {
					newLeaf = ((List<?>)leafObject).get( i );
				} catch (IndexOutOfBoundsException e) {
					//
				}
			} else {
				if (leafObject instanceof Map) {
					leafObject = ((Map<?,?>)leafObject).entrySet(  );
				}
				if (leafObject instanceof Collection) {
					int i = toPropertyIndex( propertyName );
					for(Iterator<?> it = ((Collection<?>)leafObject).iterator(); i > -1; i--) {
						try {
							newLeaf = it.next();
						} catch (NoSuchElementException e) {
							newLeaf = null;
							break;
						}
					}
				} else {
					try {
						BeanInfo beanInfo = Introspector.getBeanInfo(leafObject.getClass( ));
						PropertyDescriptor[] props = beanInfo.getPropertyDescriptors();
						for (PropertyDescriptor prop : props) {
							if (prop.getName().equals(propertyName)) {
								Method getter = prop.getReadMethod();
								// TODO: indexed properties should be handled by putting IPD to the stack
								// TODO: enumerating IPD is calling getter until out of bounds exception
								if (getter.getParameterCount() == 0) {
									newLeaf = getter.invoke( leafObject );
								}
							}
						}
					} catch (IllegalAccessException e) {
					    //
					} catch (InvocationTargetException e) {
					    //
					} catch (IntrospectionException e) {
						//
					}
				}
			}
			//if (propertyType == null && newLeaf ) {
			//}
			// 
		}
		pathEntries.add( newLeaf );
		//PropertyEditorManager.findEditor( targetType )
		//java.beans.Beans x = null;
		//BeanInfo y = null;
		//PropertyEditor z = null;
		//z.setAsText( "" );
		//z.getas
		//y.getPropertyDescriptors()[0].getWriteMethod(  );
	}

	private static final Logger LOGGER = Logger.getLogger( Editor.class.getName() );
}
