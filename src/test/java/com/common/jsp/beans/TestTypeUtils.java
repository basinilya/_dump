package com.common.jsp.beans;

import java.lang.reflect.Field;
import java.lang.reflect.Type;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.Collection;

import junit.framework.TestCase;

import com.google.common.reflect.TypeToken;

@SuppressWarnings( { "serial", "rawtypes", "unchecked" } )
public class TestTypeUtils
    extends TestCase
{
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

    public static Collection<Timestamp> fld;

    public void testAssignable()
        throws Exception
    {
        // bad: fld = new MyCollection1();
        fld = new MyCollection2();
        fld = new MyCollection3(); // just a warning
        fld = new MyCollection4(); // just a warning

        Field getter = TestTypeUtils.class.getField( "fld" );
        Type memberType = getter.getGenericType();

        TypeToken<?> resolved = TypeToken.of( TestTypeUtils.class ).resolveType( memberType );
        System.out.println( "type of fld: " + resolved );

        assertTrue( checkAssignable( resolved, ArrayList.class ) );
        assertFalse( checkAssignable( resolved, MyCollection1.class ) );
        assertTrue( checkAssignable( resolved, MyCollection2.class ) );
        assertTrue( checkAssignable( resolved, MyCollection3.class ) );
        assertTrue( checkAssignable( resolved, MyCollection4.class ) );
    }

    private static boolean checkAssignable( TypeToken<?> resolved, Class<?> implClass )
    {
        boolean res = TypeUtils.isAssignable( resolved, implClass );
        System.out.println( "fld = new " + implClass.getSimpleName() + "() -> " + ( res ? "yes" : "no" ) );
        return res;
    }

}