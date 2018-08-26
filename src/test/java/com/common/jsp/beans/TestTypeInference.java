package com.common.jsp.beans;

import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.GenericDeclaration;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Proxy;
import java.lang.reflect.TypeVariable;
import java.sql.Timestamp;
import java.util.*;
import java.util.function.Function;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.lang.reflect.Type;
import java.lang.reflect.WildcardType;
import java.net.URL;
import java.net.URLClassLoader;

import org.checkerframework.checker.nullness.qual.Nullable;

import typetoken.getsubtype.test.HashMap_Integer_Number;
import typetoken.getsubtype.test.HashMap_Integer_Timestamps;
import typetoken.getsubtype.test.HashMap_Integer_V;
import typetoken.getsubtype.test.HashMap_Integer_VNumber;
import typetoken.getsubtype.test.HashMap_Integer_VTimestamps;
import typetoken.getsubtype.test.HashMap_Raw;

import com.common.jsp.beans.TestTypeInference.ClassWithWildcardParameter.MapImpl;
import com.google.common.reflect.ClassPath;
import com.google.common.reflect.TypeToken;
import com.google.common.reflect.ClassPath.ClassInfo;
import com.google.common.reflect.ClassPath.ResourceInfo;

import junit.framework.TestCase;

@SuppressWarnings( "serial" )
public class TestTypeInference
    extends TestCase
{
    // our targs[0] actually becomes Map's targs[1]
    public interface MyMap<A>
        extends Map<Integer, A>
    {
    }

    public static abstract class ClassWithWildcardParameter<V>
    {
        public MyMap<V> myMap;

        public Map<Integer, V> map;

        class MapImpl
            extends HashMap<Integer, V>
            implements MyMap<V>
        {
        };
    }

    // this class provides enough runtime info to know that getIncomplete() returns Map<Integer,Timestamp>
    public static class CompleteClass
        extends ClassWithWildcardParameter<Collection<Timestamp>>
    {
    }

    @SuppressWarnings( { "unchecked", "rawtypes" } )
    private static TypeToken getSubtype(TypeToken resolvedSuperTt, Class<?> subclass) {
        try {
            TypeToken res = resolvedSuperTt.getSubtype( subclass );
            System.out.println(res);
            return res;
        } catch (IllegalArgumentException e) {
            return null;
        }
    }
    
    @SuppressWarnings( { "unused", "rawtypes", "unchecked" } )
    public <Z> void testIncomplete()
        throws Exception
    {
        Field member = CompleteClass.class.getField( "map" );
        GenericDeclaration altGd = member.getDeclaringClass();
        Type memberType = member.getGenericType();

        CompleteClass inst = new CompleteClass();

        Map<Integer, Collection<Timestamp>> m1 = inst.myMap;
        MyMap<Collection<Timestamp>> m2 = inst.myMap;

        Map<Integer, Collection<Timestamp>> m3 = inst.map;
        // MyMap<Collection<Timestamp>> m4 = inst.map2;

        inst.myMap = inst.new MapImpl();
        inst.map = inst.new MapImpl();

        TypeToken ccTt = TypeToken.of( CompleteClass.class );
        TypeToken resolvedSuperTt = ccTt.resolveType( memberType );

        Class implClass = HashMap.class;
        implClass = ClassWithWildcardParameter.MapImpl.class;

        TypeToken resolvedImplTt; // = resolvedSuperTt.getSubtype( implClass );
        resolvedImplTt = getSubtype( resolvedSuperTt, HashMap.class );
        resolvedImplTt = getSubtype( resolvedSuperTt, HashMap_Integer_V.class );
        resolvedImplTt = getSubtype( resolvedSuperTt, HashMap_Integer_Timestamps.class );
        resolvedImplTt = getSubtype( resolvedSuperTt, HashMap_Integer_VTimestamps.class );
        resolvedImplTt = getSubtype( resolvedSuperTt, HashMap_Integer_VNumber.class );
        resolvedImplTt = getSubtype( resolvedSuperTt, HashMap_Raw.class );
        resolvedImplTt = getSubtype( resolvedSuperTt, HashMap_Integer_Number.class );
        resolvedImplTt.toString();

        
        //Type resolvedSuperType = resolvedSuperTt.getType();


        //TypeToken implClassTt = TypeToken.of( implClass );

        //TypeToken unresolvedSuperTt = implClassTt.getSupertype( HashMap.class );
        //TypeToken asasd = ccTt.resolveType( TypeUtils.wrap( altGd, unresolvedSuperTt.getType() ) );

        //asasd.toString();
    }

    private static <T> ParameterizedType resolveParamsFor( Class<T> target, Class<?> completeClazz, Type memberType )
    {
        TypeToken<?> tt = TypeToken.of( completeClazz );
        @SuppressWarnings( "unchecked" )
        TypeToken<T> resolved = (TypeToken<T>) tt.resolveType( memberType );
        TypeToken<? super T> sup = resolved.getSupertype( target );
        Type x = sup.getType();
        return (ParameterizedType) x;
    }

    public static class MyCollection1
        extends ArrayList<Integer>
    {
    }

    public static class MyCollection2
        extends ArrayList<Timestamp>
    {
    }

    public static class MyCollection3<T>
        extends ArrayList<T>
    {
    }

    public static class MyCollection4
        extends ArrayList
    {
    }
}
