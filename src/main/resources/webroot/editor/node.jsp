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
					<c:set var="nullable" value="${!factoryProvider.typeToken.primitive}" />
					<c:set var="paramName" value="${prefix}null"/>
					<c:set var="param_null" value="${param[paramName] && nullable}"/>
					<label>
						<input type="radio" name="${prefix}null" value="true"<c:if test="${!nullable}"> disabled="disabled"</c:if><c:if test="${param_null}"> checked="checked"</c:if>>
						Null
					</label>
					<label>
						<input type="radio" name="${prefix}null" value="false"<c:if test="${!param_null}"> checked="checked"</c:if>>
						Not null:
					</label>
					</div>
					<div>
					<c:set var="paramName" value="${prefix}restrict"/>
					<c:set var="param_restrict" value="${param[paramName]}"/>
					<c:set var="param_restrict" value="${empty param_restrict ? '' : param_restrict}"/>
					<c:set var="factories" value="${factoryProvider.factories[param_restrict]}"/>
					<br/>
					<c:set var="paramName" value="${prefix}iFactory"/>
					<c:set var="param_iFactory" value="${param[paramName]}"/>
					<c:set var="param_iFactory" value="${param_iFactory ge fn:length(factories) ? null : param_iFactory}"/>
					<input type="hidden" name="${prefix}oldIFactory" value="${fn:escapeXml(param_iFactory)}"/>
					<label>
						Factory:
						<select name="${prefix}iFactory">
							<c:set var="i" value="0"/>
							<c:forEach var="i" begin="1" end="${fn:length(factories)}" step="1">
								<option value="${i - 1}"<c:if test="${param_iFactory == (i-1)}"> selected="selected"</c:if>>
									<c:out value="${factories[i-1]}" />
								</option>
							</c:forEach>
						</select>
					</label>
					<input type="hidden" name="${prefix}oldRestrict" value="${fn:escapeXml(param_restrict)}"/>
					<br/>
					<label>
						Restrict:
						<input type="text" name="${prefix}restrict" value="${fn:escapeXml(param_restrict)}"/>
					</label>
					</div>
					<c:set var="factory" value="${factories[param_iFactory+0]}"/>
					<c:set var="tags" value="${factory.tags}"/>
					<c:choose>
						<c:when test="${tags != null}">
							<c:forEach var="i" begin="1" end="${fn:length(tags)}" step="1">
								<label>
									<input type="radio" name="${prefix}iTag" value="${i-1}"<c:if test="${param.iTag == (i-1)}"> checked="checked"</c:if>/>
									<c:out value="${tags[i-1]}"/><br/>
								</label>
							</c:forEach>
						</c:when>
						<c:when test="${factoryProvider.typeToken == 'java.lang.String'
							&& fn:length(factory.paramsProviders) == 1
							&& factory.paramsProviders[0].typeToken == 'java.lang.String'}">
							<c:set var="paramName" value="${prefix}value"/>
							<c:set var="param_value" value="${param[paramName]}"/>
							<label>
								Value:
								<input type="text" name="${prefix}value" value="${fn:escapeXml(param_value)}" />
							</label>
						</c:when>
						<c:otherwise>
							<c:choose>
								<c:when test="${depth gt 20}">error</c:when>
								<c:otherwise>
									<c:set var="paramsProviders" value="${factory.paramsProviders}"/>
									<c:set scope="page" var="saveDepth" value="${depth}"/>
									<c:set scope="page" var="savePrefix" value="${prefix}"/>
									<c:forEach var="i" begin="1" end="${fn:length(paramsProviders)}" step="1">
										<c:set scope="request" var="depth" value="${saveDepth+1}"/>
										<c:set scope="request" var="factoryProvider" value="${paramsProviders[i-1]}"/><%--
										--%><c:set scope="request" var="prefix" value="${savePrefix}arg${i-1}-"/><%--
										--%><c:set scope="request" var="legend" value="${paramsProviders[i-1].typeToken} arg${i-1}"/>
										<jsp:include page="node.jsp"/>
									</c:forEach>
								</c:otherwise>
							</c:choose>
						</c:otherwise>
					</c:choose>
				</fieldset>
