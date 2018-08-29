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
--%><html>
	<head></head>
	<body>
		<%
			class Helper {
				Helper(final HttpServletRequest request, final HttpServletResponse response, final PageContext pageContext, final JspWriter out) {
					this.request = request;
					this.response = response;
					this.pageContext = pageContext;
					this.out = out;
					this._jspx_page_context = pageContext;
				}
			
				final HttpServletRequest request;
				final HttpServletResponse response;
				final JspContext pageContext;
				final JspWriter out;
				final PageContext _jspx_page_context;

				boolean prmat;

				void mkUrl()
					throws Throwable
				{prmat = true;
					%><%--
					--%><c:url var="url" value=""><%--
						--%><c:param name="n" value="${mkUrl_i-((empty mkUrl_extra) ? 1 : 0)}"/><%--
						--%><c:forEach begin="1" end="${mkUrl_i-1}" step="1" var="mkUrl_j"><%--
							--%><c:param name="p${mkUrl_j}" value="${param['p'.concat(mkUrl_j)]}"/><%--
						--%></c:forEach><%--
						--%><c:if test="${not empty mkUrl_extra}"><c:param name="p${mkUrl_i}" value="${mkUrl_extra}"/></c:if><%--
					--%></c:url><%--
					--%><% prmat = false;
				}
			}
			final Helper helper = new Helper(request, response, pageContext, out);
			pageContext.setAttribute("clazzMap", Map.class);
		%>
		<c:set var="rootPojo" value="${fastBean.v24Config.f}"/>
		<c:set var="pojo" value="${rootPojo}"/>
		<c:set var="lastpropname" value="f"/>
		<c:set var="parent" value=""/>
		<h1>
			<c:forEach begin="1" end="${param.n}" step="1" var="mkUrl_i"><%--
				--%><c:set var="propname" value="${param['p'.concat(mkUrl_i)]}"/><%--
				--%><% helper.mkUrl(); if (helper.prmat) return; %><%--
				--%><%--
				--%><a href="${url}"><c:out value="${lastpropname}"/></a>.<%--
				--%><c:set var="lastpropname" value="${propname}"/><%--
				--%><c:set var="parent" value="${parent}.${propname}"/><%--
				--%><c:set var="parentPojo" value="${pojo}"/><%--
				--%><c:set var="pojo" value="${pojo[propname]}"/><%--
			--%></c:forEach><%--
			--%><c:out value="${lastpropname}"/>
		</h1>
		<h3>Value</h3>
		<c:out value="(${fn:substring(pojo,0,100)})"/>

		<h3>Properties</h3>
		<c:set var="pojoClass" value="${pojo.getClass()}"/>
		<table border="1">
			<thead>
				<tr>
					<th>property</th>
					<th>proptype</th>
					<th>value</th>
				</tr>
			</thead>
			<tbody>
				<c:choose>
					<c:when test="${pojo == null}">
						
					</c:when>
					<c:when test="${clazzMap.isAssignableFrom(pojoClass)}">
						<%
							java.lang.reflect.Type valType = null;
							try {
								PropertyDescriptor[] descriptors =
										Introspector.getBeanInfo(pageContext.getAttribute("parentPojo").getClass()).getPropertyDescriptors();
								String lastpropname = (String)pageContext.getAttribute("lastpropname");
								for (PropertyDescriptor descriptor : descriptors) {
									if (descriptor.getName().equals(lastpropname)) {
										java.lang.reflect.Method getter = descriptor.getReadMethod();
										java.lang.reflect.ParameterizedType gt = (java.lang.reflect.ParameterizedType) getter.getGenericReturnType();
										valType = gt.getActualTypeArguments()[1];
										if (valType instanceof java.lang.reflect.WildcardType) {
											valType = ((java.lang.reflect.WildcardType)valType).getUpperBounds()[0];
										}
										break;
									}
								}
							} catch (Exception e) {
							}
							if (!(valType instanceof Class)) {
								valType = null;
							}
							pageContext.setAttribute("valType", valType);
						%>
						<c:forEach items="${pojo}" var="entry">
							<tr>
								<td><c:out value="${entry.key}"/></td>
								<td><c:out value="${valType.name.replaceFirst('.*[.]','')}"/></td>
								<td><c:out value="${fn:substring(entry.value,0,100)}"/></td>
							</tr>
						</c:forEach>
			<form name="aaa" method="get">
				<input name="add" type="submit" value="add">
			</form>
					</c:when>
					<c:otherwise>
						<%
							pageContext.setAttribute("descriptors",
									Introspector.getBeanInfo(pageContext.getAttribute("pojo").getClass()).getPropertyDescriptors());
						%>
						<c:forEach items="${descriptors}" var="descriptor">
							<c:if test="${descriptor.name != 'class'}">
								<c:set var="mkUrl_extra" value="${descriptor.name}"/>
								<c:set var="proptype" value="${descriptor.propertyType}"/>
								<c:set var="value" value="${pojo[mkUrl_extra]}"/>
								<tr>
									<td>
										<c:out value="${mkUrl_extra}"/>
									</td>
									<td>
										<c:out value="${proptype.name.replaceFirst('.*[.]','')}"/>
									</td>
									<td>
										<c:set var="mkUrl_i" value="${param.n+1}"/>
										<% helper.mkUrl(); if (helper.prmat) return; %>
										<a href="${url}">
											<c:out value="(${fn:substring(value,0,100)})"/>
										</a>
									</td>
								</tr>
							</c:if>
						</c:forEach>
					</c:otherwise>
				</c:choose>
			</tbody>
		</table>
	</body>
</html>