package com.common.jsp.beans;

import java.lang.reflect.GenericArrayType;
import java.lang.reflect.GenericDeclaration;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.lang.reflect.TypeVariable;
import java.lang.reflect.WildcardType;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.HashSet;
import java.util.Set;

import com.google.common.reflect.TypeToken;

public class TypeUtils
{
    private static boolean isAssignable(TypeToken<?> resolved, Class<?> implClass) {
        return resolved.isSupertypeOf(implClass) || isAnySupertypeAssignable(resolved, implClass);
    }

    private static boolean isAnySupertypeAssignable(TypeToken<?> resolved, Class<?> implClass) {
        return TypeToken.of(implClass).getTypes().stream()
                .anyMatch(supertype -> isUncheckedSupertype(resolved, supertype));
    }

    private static boolean isUncheckedSupertype(TypeToken<?> resolved, TypeToken<?> implTypeToken) {
        if (implTypeToken.getType() instanceof ParameterizedType) {
            return false; // this prevents assignments of Collection<Integer> to Collection<Timestamp>
        }
        try {
            resolved.getSubtype(implTypeToken.getRawType());
            return true;
        } catch (IllegalArgumentException ex) {
            return false;
        }
    }
    
    public static Type[] wrap( GenericDeclaration altGD, Type[] ta )
    {
        if ( ta != null )
        {
            for ( int i = 0; i < ta.length; i++ )
            {
                ta[i] = wrap( altGD, ta[i] );
            }
        }
        return ta;
    }

    public static Type wrap( GenericDeclaration altGD, Type t )
    {
        if ( t instanceof GenericArrayType )
        {
            return new GenericArrayTypeImpl( altGD, (GenericArrayType) t );
        }
        if ( t instanceof ParameterizedType )
        {
            return new ParameterizedTypeImpl( altGD, (ParameterizedType) t );
        }
        if ( t instanceof TypeVariable )
        {
            return new TypeVariableImpl<>( altGD, (TypeVariable<?>) t );
        }
        if ( t instanceof WildcardType )
        {
            return new WildcardTypeImpl( altGD, (WildcardType) t );
        }
        return t;
    }

    public static <T> boolean checkAssignable2( TypeToken<? super T> resolvedSuperTt, Class<T> implClass )
    {
        return false;
    }

    public static <T> boolean checkAssignable( TypeToken<? super T> resolvedSuperTt, Class<T> implClass )
    {
        // System.out.println( "fld = new " + implClass.getSimpleName() + "()" );
        boolean b = resolvedSuperTt.isSupertypeOf( implClass );

        if ( !b && ( resolvedSuperTt.getType() instanceof ParameterizedType ) )
        {

            ParameterizedType resolvedSuperType = (ParameterizedType) resolvedSuperTt.getType();

            TypeToken<T> xxx = TypeToken.of( implClass );
            // System.out.println(xxx);

            TypeToken<? super T> unresolvedSuperTt = xxx.getSupertype( resolvedSuperTt.getRawType() );
            // System.out.println(yyy);

            // TypeToken<?> zzz = resolveContext.resolveType( yyy.getType() );
            // System.out.println(zzz);
            // yyy.get

            if ( !( unresolvedSuperTt.getType() instanceof ParameterizedType ) )
            {
                throw new UnsupportedOperationException( "Don't know how to handle: " + unresolvedSuperTt );
            }

            ParameterizedType unresolvedSuperType = (ParameterizedType) unresolvedSuperTt.getType();

            b = true;

            Type[] resolvedTArgs = resolvedSuperType.getActualTypeArguments();

            Type[] unresolvedTArgs = unresolvedSuperType.getActualTypeArguments();
            for ( int i = 0; i < unresolvedTArgs.length; i++ )
            {
                Type unresolvedTArg = unresolvedTArgs[i];
                Type resolvedTArg = resolvedTArgs[i];
                // TypeToken<?> targTt = TypeToken.of( targ );
                // boolean c = targTt.isSupertypeOf( targs2[i] );
                if ( unresolvedTArg instanceof TypeVariable )
                {
                    TypeVariable<?> tvar = (TypeVariable<?>) unresolvedTArg;
                    Type[] bounds = tvar.getBounds();
                    if ( bounds.length != 1 || !( bounds[0] instanceof Class ) )
                    {
                        // GenericDeclaration gdecl = tvar.getGenericDeclaration();
                        // if (!targ.equals( targs2[i] )) {
                        throw new UnsupportedOperationException( "Don't know how to handle: " + unresolvedTArg );
                    }
                    if ( !( (Class<?>) bounds[0] ).isAssignableFrom( (Class<?>) resolvedTArgs[i] ) )
                    {
                        b = false;
                    }
                }
                else if ( unresolvedTArg instanceof Class )
                {
                    if ( !unresolvedTArg.equals( resolvedTArgs[i] ) )
                    {
                        if ( !( resolvedTArgs[i] instanceof Class ) )
                        {
                            throw new UnsupportedOperationException( "Don't know how to handle: " + unresolvedTArg );
                        }
                        b = false;
                    }
                }
                else
                {
                    throw new UnsupportedOperationException( "Don't know how to handle: " + unresolvedTArg );
                }
            }
        }

        // System.out.println(b ? "yes" : "no" );
        return b;
    }
    
    public static ClassLoader mkScannableClassLoader(ClassLoader parent) {
        if (Object.class.getClassLoader() == null) {
            try {
                URL url = Object.class.getResource("Object.class");
                String s = url.toString().replaceFirst( "^jar:(.*)![^!]*$", "$1");
                url = new URL(s);
                URLClassLoader ucl = new URLClassLoader( new URL[] { url } , parent );
                return ucl;
            } catch (Throwable e) {
                //
            }
        }
        return parent;
    }
}
