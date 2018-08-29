<%@ page language="java" contentType="text/html; charset=UTF-8" pageEncoding="UTF-8"%><%--
--%><%@page import="java.beans.*"%><%--
--%><%@ page import="java.util.*" %><%--
--%><%@ page import="java.lang.reflect.*" %><%--
--%><%@ page import="java.util.logging.Level" %><%--
--%><%@ page import="java.util.logging.Logger" %><%--
--%><%@ page import="javax.servlet.*" %><%--
--%><%@ page import="javax.servlet.http.*" %><%--
--%><%@ taglib prefix = "c" uri = "http://java.sun.com/jsp/jstl/core" %><%--
--%><%@ taglib prefix = "fmt" uri = "http://java.sun.com/jsp/jstl/fmt" %><%--
--%><%@ taglib prefix = "fn" uri = "http://java.sun.com/jsp/jstl/functions" %><%--
--%><%--
--%><%!

public static class Editor
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

	public Object getLeafObjectAsText() {
	    String res = null;
		Object leafObject = getLeafObject();
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


%><%--
--%><%--
--%><%--
--%><%--
--%><%

Object rootPojo = request.getSession( ).getAttribute( "rootPojo" );
if (rootPojo == null) {
	rootPojo = new com.common.test.v24.GtwayV24Data();
	request.getSession( ).setAttribute( "rootPojo", rootPojo );
}

Editor editor = new Editor();
request.setAttribute("editor", editor);

editor.setRootObject( rootPojo );

int n = 0;
try {
	n = Byte.parseByte( request.getParameter( "n" ) );
} catch (NumberFormatException e) {
	//
}

for (int i = 0; i < n; i++) {
	String pathEntryName = request.getParameter( "p" + i );
	editor.addPathEntry( pathEntryName );
}



%><%--
--%><%--
--%><html>
	<head></head>
	<body>
		<h1>
		<c:set var="lastPathEntry" value="(root)"/>
		<c:forEach begin="1" end="${fn:length(editor.pathEntryNames)}" step="1" var="mkUrl_i"><%--
		--%><c:url var="url" value=""><%--
			--%><c:param name="n" value="${mkUrl_i-1}"/><%--
			--%><c:forEach begin="1" end="${mkUrl_i-1}" step="1" var="mkUrl_j"><%--
				--%><c:param name="p${mkUrl_j-1}" value="${editor.pathEntryNames[mkUrl_j-1]}"/><%--
			--%></c:forEach><%--
		--%></c:url><%--
		--%><a href="${url}"><c:out value="${lastPathEntry}"/></a>.<%--
		--%><%--
		--%><%--
		--%><c:set var="lastPathEntry" value="${editor.pathEntryNames[mkUrl_i-1]}"/><%--
		--%></c:forEach><%--
		--%><c:out value="${lastPathEntry}"/>
		</h1>
		<form method="post">
			<div>
			<input type="submit" name="remove" value="remove element" <c:if test="${true}">disabled="disabled"</c:if> />
			<input type="submit" name="assign" value="create/assign" <c:if test="${true}">disabled="disabled"</c:if> />
			</div>
			<div>
			Value: <c:out value="(${fn:substring(editor.leafObjectAsText,0,100)})"/>
			</div>
		</form>
	</body>
</html>
