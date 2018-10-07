<%@ page language="java" contentType="text/html; charset=UTF-8" pageEncoding="UTF-8"%><%--
--%><%@ page import="java.util.*" %><%--
--%><%@ page import="com.common.jsp.beans.*" %><%--
--%><%@ page import="com.common.test.v24.GtwayV24Data" %><%--
--%><%@ taglib prefix = "c" uri = "http://java.sun.com/jsp/jstl/core" %><%--
--%><%@ taglib prefix = "fmt" uri = "http://java.sun.com/jsp/jstl/fmt" %><%--
--%><%@ taglib prefix = "fn" uri = "http://java.sun.com/jsp/jstl/functions" %><%--
--%><%--
--%><%!

	/** Helps when just "?a=b" turns into ";jsessionId=blah?a=b" */
	public static String getOriginalRequestURI(HttpServletRequest request) {
		String uri = (String) request.getAttribute("javax.servlet.forward.request_uri");
		return uri == null ? request.getRequestURI() : uri;
	}

	private static boolean isBlank(String s) {
		return s == null || s.isEmpty();
	}

%><%--
--%><%

GtwayV24Data rootPojo;

if (!isBlank(request.getParameter("discard")) || null == request.getSession( ).getAttribute("rootPojoSet")) {
	rootPojo = TestService.getGtwayV24Data();
	request.getSession( ).setAttribute( "rootPojoSet", true );
	request.getSession( ).setAttribute( "rootPojo", rootPojo );
} else {
	rootPojo = (GtwayV24Data)request.getSession( ).getAttribute( "rootPojo" );
	if (!isBlank(request.getParameter("save"))) {
		TestService.setGtwayV24Data(rootPojo);
	} else if ("POST".equals(request.getMethod())) {
		if (!isBlank(request.getParameter("editor"))) {
			request.getSession( ).setAttribute( "rootPojo", rootPojo );
			request.getSession( ).setAttribute( "back", getOriginalRequestURI(request) );
			response.sendRedirect("editor/editor.jsp");
		}
	}
}

%><%--
--%><html>
<head>
  <title>Embedded Jetty: JSP Examples</title>
</head>
<body>
  <h1>Embedded Jetty: JSP Examples</h1>

  <p>
<a href="who.jsp">who.jsp</a>
<br/>

	Value: <c:out value="(${rootPojo})"/>

	<form method="post">
		<div>
		<input type="submit" name="editor" value="edit"/>
		<input type="submit" name="save" value="save"/>
		<input type="submit" name="discard" value="discard"/>
		</div>
	</form>
</body>
</html>