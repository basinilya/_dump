package com.common.jsp.beans;

import java.lang.reflect.GenericDeclaration;
import java.lang.reflect.Type;
import java.lang.reflect.WildcardType;

public class WildcardTypeImpl implements WildcardType
{
    private final WildcardType inst;
    private final Type[] lowerBounds;
    private final Type[] upperBounds;

    public WildcardTypeImpl(GenericDeclaration altGD, WildcardType inst) {
        this.inst = inst;
        lowerBounds = TypeUtils.wrap( altGD, inst.getLowerBounds() );
        upperBounds = TypeUtils.wrap( altGD, inst.getUpperBounds() );
    }

    @Override
    public String toString()
    {
        return inst.toString();
    }

    @Override
    public Type[] getUpperBounds()
    {

        return upperBounds.clone();
    }

    @Override
    public Type[] getLowerBounds()
    {

        return lowerBounds.clone();
    }
}
