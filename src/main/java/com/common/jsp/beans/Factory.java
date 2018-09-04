package com.common.jsp.beans;

import java.util.List;

public abstract class Factory {
	
	public abstract Object getInstance(Object[] params) throws Exception;
	
	public abstract List<FactoryProvider> getParamsProviders();
	//public static 
}
