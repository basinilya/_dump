package com.common.jsp.beans;

import javax.servlet.http.HttpSession;

import com.google.common.reflect.TypeToken;

public class SessionHusk extends BeanHusk {
	private HttpSession session;

	public SessionHusk(HttpSession request) {
		this.session = request;
	}

	public SessionHusk() {
	}

	public HttpSession getSession() {
		return session;
	}
	
	@Override
	protected Object getValue0() {
		return session.getAttribute(ATTRNAME);
	}

	@Override
	public TypeToken getTypeToken() {
		return TypeToken.of(Object.class);
	}
	
	@Override
	public void setValue(Object value) {
		session.setAttribute(ATTRNAME, value);
	}

	private static final String ATTRNAME = "rootPojo";

	public void setSession(HttpSession request) {
		// request.getSession();
		this.session = request;
	}
}
