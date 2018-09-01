<%@ page language="java" contentType="text/html; charset=UTF-8" pageEncoding="UTF-8"%><%--
--%><%@page import="java.beans.*"%><%--
--%><%@ page import="java.util.*" %><%--
--%><%@ page import="java.lang.reflect.*" %><%--
--%><%@ page import="java.util.logging.Level" %><%--
--%><%@ page import="java.util.logging.Logger" %><%--
--%><%@ page import="javax.servlet.*" %><%--
--%><%@ page import="javax.servlet.http.*" %><%--
--%><%@ page import="com.common.jsp.beans.TestService" %><%--
--%><%@ page import="com.google.common.reflect.TypeToken" %><%--
--%><%@ taglib prefix = "c" uri = "http://java.sun.com/jsp/jstl/core" %><%--
--%><%@ taglib prefix = "fmt" uri = "http://java.sun.com/jsp/jstl/fmt" %><%--
--%><%@ taglib prefix = "fn" uri = "http://java.sun.com/jsp/jstl/functions" %><%--
--%><%--
--%><%!

%><%--
--%><%--
--%><%--
--%><%--
--%><%

Object rootPojo = request.getSession( ).getAttribute( "rootPojo" );
if (rootPojo == null) {
	rootPojo = TestService.getGtwayV24Data();
	request.getSession( ).setAttribute( "rootPojo", rootPojo );
}

%><%--
--%><%--
--%><html>
	<head></head>
	<body>
		<jsp:useBean id="rootHusk" scope="request" class="com.common.jsp.beans.BeanHusk"/>
		<c:set target="${rootHusk}" property="value" value="${rootPojo}" />
		<c:set var="leafHusk" value="${rootHusk}" />
		<h1>
		<c:set var="lastPathEntry" value="(root)"/>
		<c:forEach begin="1" end="${param.n}" step="1" var="mkUrl_i"><%--
		--%><%--
		--%><%--
		--%><c:url var="url" value=""><%--
			--%><c:param name="n" value="${mkUrl_i-1}"/><%--
			--%><c:forEach begin="1" end="${mkUrl_i-1}" step="1" var="mkUrl_j"><%--
				--%><%--
				--%><c:set var="paramName" value="p${mkUrl_j-1}" /><%--
				--%><c:param name="${paramName}" value="${param[paramName]}"/><%--
			--%></c:forEach><%--
		--%></c:url><%--
		--%><a href="${url}"><c:out value="${lastPathEntry}"/></a>.<%--
		--%><%--
		--%><%--
		--%><%--
		--%><c:set var="paramName" value="p${mkUrl_i-1}" /><%--
		--%><c:set var="lastPathEntry" value="${param[paramName]}"/><%--
		--%><c:set var="leafHusk" value="${leafHusk.properties[lastPathEntry]}" /><%--
		--%></c:forEach><%--
		--%><c:out value="${lastPathEntry}"/>
		</h1>
		<form method="post">
			<div>
			<input type="submit" name="remove" value="remove element" <c:if test="${true}">disabled="disabled"</c:if> />
			<input type="submit" name="assign" value="create/assign" <c:if test="${true}">disabled="disabled"</c:if> />
			</div>
			<div>
			Value: <c:out value="(${fn:substring(leafHusk.valueAsText,0,100)})"/>
			</div>
		</form>

		<table border="1">
			<thead>
				<tr>
					<th>property</th>
					<th>proptype</th>
					<th>value</th>
				</tr>
			</thead>
			<tbody>

		<c:forEach items="${leafHusk.properties}" var="rowEntry">
			<c:set var="row" value="${rowEntry.value}"/>
			<c:set var="url" value=""/>
			<c:if test="${row.type.rawType.name != 'java.lang.Class'}">
				<c:url var="url" value=""><%--
					--%><c:param name="n" value="${param.n+1}"/><%--
					--%><c:forEach begin="1" end="${param.n}" step="1" var="mkUrl_j"><%--
						--%><%--
						--%><c:set var="paramName" value="p${mkUrl_j-1}" /><%--
						--%><c:param name="${paramName}" value="${param[paramName]}"/><%--
					--%></c:forEach><%--
					--%><c:param name="p${param.n+0}" value="${rowEntry.key}"/><%--
			--%></c:url>
			</c:if>
			<tr>
				<td>
					<c:if test="${not empty url}">
					<a href="${url}">
					</c:if>
					<c:out value="${fn:substring(row.displayName,0,100)}"/>
					<c:if test="${not empty url}">
					</a>
					</c:if>
				</td>
				<td><c:out value="${fn:substring(row.typeName,0,100)}"/></td>
				<td>
					<c:if test="${not empty url}">
					(<a href="${url}">
					</c:if>
					<c:set var="valueAsText" value="${row.valueAsText}"/>
					<c:out value="${fn:substring((valueAsText == null ? 'null' : valueAsText),0,100)}"/>
					<c:if test="${not empty url}">
					</a>)
					</c:if>
				</td>
			</tr>
		</c:forEach>

			</tbody>
		</table>

	</body>
</html>
