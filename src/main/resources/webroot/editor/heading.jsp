<%@ page language="java" contentType="text/html; charset=UTF-8" pageEncoding="UTF-8"%><%--
--%><%@ page import="java.util.*" %><%--
--%><%@ page import="java.lang.reflect.*" %><%--
--%><%@ page import="javax.servlet.*" %><%--
--%><%@ page import="javax.servlet.http.*" %><%--
--%><%@ page import="java.beans.*" %><%--
--%><%@ taglib prefix = "c" uri = "http://java.sun.com/jsp/jstl/core" %><%--
--%><%@ taglib prefix = "fmt" uri = "http://java.sun.com/jsp/jstl/fmt" %><%--
--%><%@ taglib prefix = "fn" uri = "http://java.sun.com/jsp/jstl/functions" %><%--
--%><%--
--%><jsp:useBean id="tmp" class="com.common.jsp.beans.BeanHusk"/><%--
--%><c:set var="rootHusk" scope="request" value="${tmp}" /><%--
--%><c:set target="${rootHusk}" property="value" value="${rootPojo}" /><%--
--%><c:set scope="request" var="leafHusk" value="${rootHusk}" /><%--
--%><c:set scope="request" var="lastPathEntry" value="(root)"/><%--
--%><c:forEach begin="1" end="${param.n}" step="1" var="mkUrl_i"><%--
	--%><%--
	--%><%--
	--%><c:url scope="request" var="url" value="${selfPath}"><%--
		--%><c:param name="n" value="${mkUrl_i-1}"/><%--
		--%><c:forEach begin="1" end="${mkUrl_i-1}" step="1" var="mkUrl_j"><%--
			--%><%--
			--%><c:set var="paramName" value="p${mkUrl_j-1}" /><%--
			--%><c:param name="${paramName}" value="${param[paramName]}"/><%--
		--%></c:forEach><%--
	--%></c:url><%--
	--%><c:if test="${empty mute}"><a href="${url}"><c:out value="${lastPathEntry}"/></a>&#8203;.</c:if><%--
	--%><%--
	--%><%--
	--%><c:set var="paramName" value="p${mkUrl_i-1}" /><%--
	--%><c:set scope="request" var="lastPathEntry" value="${param[paramName]}"/><%--
	--%><c:set scope="request" var="leafHusk" value="${leafHusk.properties[lastPathEntry]}" /><%--
	--%></c:forEach><%--
--%><c:if test="${empty mute}"><c:out value="${lastPathEntry}"/></c:if>