package com.common.jsp.beans;

import java.lang.reflect.GenericArrayType;
import java.lang.reflect.GenericDeclaration;
import java.lang.reflect.Modifier;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.lang.reflect.TypeVariable;
import java.lang.reflect.WildcardType;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.HashSet;
import java.util.Set;

import com.google.common.reflect.ClassPath;
import com.google.common.reflect.TypeToken;
import com.google.common.reflect.ClassPath.ClassInfo;

public class TypeUtils
{
    public static boolean isAssignable( TypeToken<?> resolved, Class<?> implClass )
    {
        return resolved.isSupertypeOf( implClass ) || isAnySupertypeAssignable( resolved, implClass );
    }

    public static boolean isAnySupertypeAssignable( TypeToken<?> resolved, Class<?> implClass )
    {
        return TypeToken.of( implClass ).getTypes().stream().anyMatch( supertype -> isUncheckedSupertype( resolved,
                                                                                                          supertype ) );
    }

    public static boolean isUncheckedSupertype( TypeToken<?> resolved, TypeToken<?> implTypeToken )
    {
        if ( implTypeToken.getType() instanceof ParameterizedType )
        {
            return false; // this prevents assignments of Collection<Integer> to Collection<Timestamp>
        }
        try
        {
            resolved.getSubtype( implTypeToken.getRawType() );
            return true;
        }
        catch ( IllegalArgumentException ex )
        {
            return false;
        }
    }

    public static Set<Class<?>> findImplementations( ClassPath classPath, String prefix, TypeToken<?> resolved, Class<?> restrict )
    {
        Class<?> clazz = restrict == null ? resolved.getRawType() : restrict;
        Set<Class<?>> actual = new HashSet<>();

        for ( ClassInfo classInfo : classPath.getAllClasses() )
        {
            if ( !classInfo.getName().startsWith( prefix ) )
            {
                continue;
            }
            final Class<?> candidate;
            try
            {
                Class<?> tmp = (Class<?>) classInfo.load();
                candidate = tmp;
            }
            catch ( Throwable e )
            {
                continue;
            }
            if ( clazz.isAssignableFrom( candidate ) && Modifier.isPublic( candidate.getModifiers() )
                && ( Modifier.isStatic( candidate.getModifiers() ) || candidate.getEnclosingClass() == null )
                && !Modifier.isAbstract( candidate.getModifiers() ) && !Modifier.isInterface( candidate.getModifiers() ) )
            {
                if ( TypeUtils.isAssignable( resolved, candidate ) )
                {
                    actual.add( candidate );
                }
            }
        }
        return actual;
    }

    public static ClassLoader mkScannableClassLoader( ClassLoader parent )
    {
        if ( Object.class.getClassLoader() == null )
        {
            try
            {
                URL url = Object.class.getResource( "Object.class" );
                String s = url.toString().replaceFirst( "^jar:(.*)![^!]*$", "$1" );
                url = new URL( s );
                URLClassLoader ucl = new URLClassLoader( new URL[] { url }, parent );
                return ucl;
            }
            catch ( Throwable e )
            {
                //
            }
        }
        return parent;
    }
}
