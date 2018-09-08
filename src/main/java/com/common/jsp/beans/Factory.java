package com.common.jsp.beans;

import java.util.List;

public abstract class Factory implements Comparable<Factory> {
	
	public abstract Object getInstance(Object[] params) throws Exception;
	
	public abstract List<FactoryProvider> getParamsProviders();

	public Object[] getTags() { return null; };

	abstract FactoryProvider getProvider();

	@Override
	public int compareTo(Factory o) {
		if (o == null) {
			return 1;
		}
		if (this.equals(o)) {
			return 0;
		}
		return ClassComparator.INSTANCE.compare(this.getClass(), o.getClass());
	}
}
