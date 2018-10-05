<%@ page import="com.common.jsp.beans.*" %><%--
--%><%
if ("POST".equals(request.getMethod())) {
	if (null != request.getParameter("editor")) {
		Object rootPojo = TestService.getGtwayV24Data();
		request.getSession( ).setAttribute( "rootPojo", rootPojo );
		response.sendRedirect("editor/editor.jsp");
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

	<form method="post">
		<div>
		<input type="submit" name="editor" value="editor.jsp"/>
		</div>
	</form>
</body>
</html>