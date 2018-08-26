package com.common.jsp.beans;

import java.lang.reflect.GenericArrayType;
import java.lang.reflect.GenericDeclaration;
import java.lang.reflect.Type;

public class GenericArrayTypeImpl implements GenericArrayType
{
    private final GenericArrayType inst;
    private final Type genericComponentType;
    public GenericArrayTypeImpl(GenericDeclaration altGD, GenericArrayType inst) {
        this.inst = inst;
        genericComponentType = TypeUtils.wrap( altGD, inst.getGenericComponentType() );
    }

    @Override
    public String toString()
    {
        return inst.toString();
    }

    @Override
    public Type getGenericComponentType()
    {
        return genericComponentType;
    }

}
