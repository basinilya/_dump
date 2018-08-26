package com.common.jsp.beans;

import java.lang.annotation.Annotation;
import java.lang.reflect.AnnotatedType;
import java.lang.reflect.GenericDeclaration;
import java.lang.reflect.Type;
import java.lang.reflect.TypeVariable;

public class TypeVariableImpl<D extends GenericDeclaration>
    implements TypeVariable<D>
{
    private final TypeVariable<D> inst;
    private final Type[] bounds;
    private final D genericDeclaration;

    @SuppressWarnings( "unchecked" )
    public TypeVariableImpl( GenericDeclaration altGD, TypeVariable<D> inst )
    {
        this.inst = inst;
        bounds = TypeUtils.wrap( altGD, inst.getBounds() );
        // this.genericDeclaration = inst.getGenericDeclaration();
        this.genericDeclaration = (D)altGD;
    }

    @Override
    public String toString()
    {
        return inst.toString();
    }

    @Override
    public Type[] getBounds()
    {
        return bounds;
    }

    @Override
    public D getGenericDeclaration()
    {
        //System.out.println( inst.getGenericDeclaration());
        return genericDeclaration;
    }

    @Override
    public <T extends Annotation> T getAnnotation( Class<T> annotationClass )
    {

        return inst.getAnnotation( annotationClass );
    }

    @Override
    public Annotation[] getAnnotations()
    {

        return inst.getAnnotations();
    }

    @Override
    public Annotation[] getDeclaredAnnotations()
    {

        return inst.getDeclaredAnnotations();
    }

    @Override
    public AnnotatedType[] getAnnotatedBounds()
    {

        return inst.getAnnotatedBounds();
    }

    @Override
    public String getName()
    {

        return inst.getName();
    }

}
