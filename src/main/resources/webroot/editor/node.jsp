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
			--%><fieldset>
					<legend><c:out value="${legend}"/></legend>
					<div>
					<c:set var="nullable" value="${!factoryProvider.type.primitive}" />
					<label>
						<input type="radio" name="null" value="true" <c:if test="${!nullable}">disabled="disabled"</c:if> >
						Null
					</label>
					<label>
						<input type="radio" name="null" value="false" checked="checked" >
						Not null:
					</label>
					</div>
					<div>
					<c:set var="restrict" value="${empty param.restrict ? '' : param.restrict}"/>
					<c:set var="factories" value="${leafHusk.factories[restrict]}"/>
					<br/>
					<label>
						Factory:
						<select name="iFactory">
							<c:set var="i" value="0"/>
							<c:forEach var="i" begin="1" end="${fn:length(factories)}" step="1">
								<option value="${i - 1}"<c:if test="${param.iFactory == (i-1)}"> selected="selected"</c:if>>
									<c:out value="${factories[i-1]}" />
								</option>
							</c:forEach>
						</select>
					</label>
					<input type="text" disabled="disabled" name="oldRestrict" value="${param.restrict}"/>
					<br/>
					<label>
						Restrict:
						<input type="text" name="restrict" />
					</label>
					</div>
					<c:set var="factory" value="${factories[param.iFactory+0]}"/>
					<c:set var="tags" value="${factory.tags}"/>
					<c:choose>
						<c:when test="${tags != null}">
							<c:forEach var="i" begin="1" end="${fn:length(tags)}" step="1">
								<label>
									<input type="radio" name="iTag" value="${i-1}"<c:if test="${param.iTag == (i-1)}"> checked="checked"</c:if>/>
									<c:out value="${tags[i-1]}"/><br/>
								</label>
							</c:forEach>
						</c:when>
						<c:otherwise>
							<c:set var="paramsProviders" value="${factory.paramsProviders}"/>
							<c:forEach var="i" begin="1" end="${fn:length(paramsProviders)}" step="1">
								<c:set scope="request" var="factoryProvider" value="${paramsProviders[i-1]}"/>
								<c:set scope="request" var="legend" value="arg${i-1}"/>
<%-- TODO: protect recursion!
								<jsp:include page="node.jsp"/>
 --%>
							</c:forEach>
						</c:otherwise>
					</c:choose>
				</fieldset>
