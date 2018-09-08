<%@ page language="java" contentType="text/html; charset=UTF-8" pageEncoding="UTF-8"%><%--
--%><%@page import="java.beans.*"%><%--
--%><%@ page import="java.util.*" %><%--
--%><%@ page import="java.lang.reflect.*" %><%--
--%><%@ page import="java.util.logging.Level" %><%--
--%><%@ page import="java.util.logging.Logger" %><%--
--%><%@ page import="javax.servlet.*" %><%--
--%><%@ page import="javax.servlet.http.*" %><%--
--%><%@ page import="com.common.jsp.beans.*" %><%--
--%><%@ page import="java.net.URLEncoder" %><%--
--%><%@ page import="com.google.common.reflect.TypeToken" %><%--
--%><%@ taglib prefix = "c" uri = "http://java.sun.com/jsp/jstl/core" %><%--
--%><%@ taglib prefix = "fmt" uri = "http://java.sun.com/jsp/jstl/fmt" %><%--
--%><%@ taglib prefix = "fn" uri = "http://java.sun.com/jsp/jstl/functions" %><%--
--%><%--
--%><%!
	private static boolean isBlank(String s) {
		return s == null || s.isEmpty();
	}

	private static Object mkNewValue(HttpServletRequest request, FactoryProvider factoryProvider, String prefix) throws Exception {
		String oldRestrict = request.getParameter(prefix + "oldRestrict");
		List<Factory> factoryList = factoryProvider.getFactories().get(oldRestrict);
		Factory factory = factoryList.get(toInt(request.getParameter(prefix + "iFactory")));
		List<FactoryProvider> paramsProviders = factory.getParamsProviders();
		if (factoryProvider.getTypeToken().getType() == String.class
				&& paramsProviders.size() == 1
				&& paramsProviders.get(0).getTypeToken().getType() == String.class) {
		}
		boolean isNull = Boolean.TRUE.toString().equals(request.getParameter(prefix + "null"));
		Object[] tags = factory.getTags();
		String value = request.getParameter(prefix + "value");
		Object result;
		if (isNull) {
			result = null;
		} else if (tags != null) {
			Object tag = tags[toInt(request.getParameter(prefix + "iTag"))];
			result = factory.getInstance(new Object[] { tag });
		} else if (factoryProvider.getTypeToken().getType() == String.class
				&& paramsProviders.size() == 1
				&& paramsProviders.get(0).getTypeToken().getType() == String.class
				)
		{
			result = factory.getInstance(new Object[] { value });
		} else {
			Object[] constructorParams = new Object[paramsProviders.size()];
			for (int i = 0; i < paramsProviders.size(); i++) {
				constructorParams[i] = mkNewValue(request, paramsProviders.get(i) , prefix + "arg" + i + "-" );
			}
			result = factory.getInstance(constructorParams);
		}
		return result;
	}

	private static int toInt(String s) {
		return s == null ? 0 : Integer.parseInt(s);
	}
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

if ("POST".equals(request.getMethod())) {
	%><c:set scope="request" var="mute" value="x"/><jsp:include page="heading.jsp"/><c:set scope="request" var="mute" value=""/><%
	BeanHusk leafHusk = (BeanHusk)request.getAttribute("leafHusk");

	String url = request.getRequestURI() + "?" + request.getQueryString();

	if (!isBlank(request.getParameter("remove"))) {
		url = request.getRequestURI() + (String)request.getAttribute("url");
		leafHusk.remove();
		response.sendRedirect(url);
		return;
	} else if (!isBlank(request.getParameter("assign"))) {
		String prefix = "";
		FactoryProvider factoryProvider = leafHusk;
		try {
			Object newVal = mkNewValue(request, factoryProvider, prefix);
			String s = request.getParameter("index");
			if (!isBlank(s)) {
				// leafHusk.getIndex();
				leafHusk.setIndex(Integer.parseInt(s));
			}
			leafHusk.setValue(newVal);
		} catch (Exception e) {
			e.printStackTrace();
			request.setAttribute("infoMessage", "Failed");
		}
		// request.setAttribute("infoMessage", "Parameters changed");
	}
	//response.sendRedirect(url);
	//return;
}

%><%--
--%><%--
--%><html>
	<head></head>
	<body>
		<h1>
			<jsp:include page="heading.jsp"/>
		</h1>
		<form method="post">
			<div>
			<input type="submit" name="remove" value="remove element" <c:if test="${!leafHusk.removeSupported}">disabled="disabled"</c:if> />
			<input type="submit" name="assign" value="create/assign" <c:if test="${!leafHusk.setValueSupported}">disabled="disabled"</c:if> />
			</div>
			<fieldset<c:if test="${false && !leafHusk.setValueSupported}"> disabled="disabled"</c:if>>
				<div>
				Value: <c:out value="(${fn:substring(leafHusk.valueAsText,0,100)})"/>
				</div>
				<label for="index">Index:</label>
				<input type="text" name="index" id="index" value="${leafHusk.index}" <c:if test="${lastPathEntry != '-1'}">disabled="disabled"</c:if> />
				<span><b><c:out value="${infoMessage}" /></b></span>
				<%--
				--%><c:set scope="request" var="factoryProvider" value="${leafHusk}"/><%--
				--%><c:set scope="request" var="prefix" value=""/><%--
				--%><c:set scope="request" var="depth" value="0"/><%--
				--%><c:set scope="request" var="legend" value="${leafHusk.typeToken} New value"/>
				<jsp:include page="node.jsp"/>
			</fieldset>
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
			<c:if test="${row.typeToken.rawType.name != 'java.lang.Class'}">
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
					<c:out value="${rowEntry.key == '-1' ? 'new...' : fn:substring(row.displayName,0,100)}"/>
					<c:if test="${not empty url}">
					</a>
					</c:if>
				</td>
				<td><c:out value="${fn:substring(row.typeName,0,100)}"/></td>
				<td>
					<c:if test="${rowEntry.key != '-1'}">
					<c:if test="${not empty url}">
					(<a href="${url}">
					</c:if>
					<c:set var="valueAsText" value="${row.valueAsText}"/>
					<c:out value="${fn:substring((valueAsText == null ? 'null' : valueAsText),0,100)}"/>
					<c:if test="${not empty url}">
					</a>)
					</c:if>
					</c:if>
				</td>
			</tr>
		</c:forEach>

			</tbody>
		</table>

	</body>
</html>
