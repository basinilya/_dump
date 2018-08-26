package com.common.jsp.beans;

import java.lang.reflect.GenericDeclaration;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;

public class ParameterizedTypeImpl
    implements ParameterizedType
{
    private final ParameterizedType inst;
    private final Type rawType;
    private final Type ownerType;
    private final Type[] actualTypeArguments;

    public ParameterizedTypeImpl( GenericDeclaration altGD, ParameterizedType inst )
    {
        this.inst = inst;
        rawType = TypeUtils.wrap( altGD, inst.getRawType() );
        ownerType = TypeUtils.wrap( altGD, inst.getOwnerType() );
        actualTypeArguments = TypeUtils.wrap( altGD, inst.getActualTypeArguments() );
    }

    @Override
    public String toString()
    {
        return inst.toString();
    }

    @Override
    public Type[] getActualTypeArguments()
    {
        return actualTypeArguments.clone();
    }

    @Override
    public Type getRawType()
    {
        return rawType;
    }

    @Override
    public Type getOwnerType()
    {
        return ownerType;
    }
}
