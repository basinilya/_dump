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
--%>
<fieldset>
	<legend>New value</legend>
	<div>
	<c:set var="nullable" value="${!factoryProvider.type.primitive}" />
	<input type="radio" name="null" value="true" id="nullTrue" <c:if test="${!nullable}">disabled="disabled"</c:if> >
	<label for="nullTrue">Null</label>
	<input type="radio" name="null" value="false" id="nullFalse" checked="checked" >
	<label for="nullFalse">Not null:</label>
	</div>
</fieldset>

