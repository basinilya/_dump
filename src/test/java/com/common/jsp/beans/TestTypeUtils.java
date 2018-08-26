package com.common.jsp.beans;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.lang.reflect.Type;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import junit.framework.TestCase;

import com.google.common.reflect.ClassPath;
import com.google.common.reflect.ClassPath.ClassInfo;
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

    @Override
    protected void setUp()
        throws Exception
    {
        super.setUp();

        Field getter = TestTypeUtils.class.getField( "fld" );
        Type memberType = getter.getGenericType();

        resolved = TypeToken.of( TestTypeUtils.class ).resolveType( memberType );
        System.out.println( "type of fld: " + resolved );

    }

    private TypeToken<?> resolved;

    public void testAssignable()
        throws Exception
    {
        // bad: fld = new MyCollection1();
        fld = new MyCollection2();
        fld = new MyCollection3(); // just a warning
        fld = new MyCollection4(); // just a warning

        assertTrue( checkAssignable( resolved, ArrayList.class ) );
        assertFalse( checkAssignable( resolved, MyCollection1.class ) );
        assertTrue( checkAssignable( resolved, MyCollection2.class ) );
        assertTrue( checkAssignable( resolved, MyCollection3.class ) );
        assertTrue( checkAssignable( resolved, MyCollection4.class ) );
    }

    public void testScanner()
        throws Exception
    {
        ClassPath classPath = ClassPath.from( Thread.currentThread().getContextClassLoader() );
        String prefix = TestTypeUtils.class.getName() + '$';

        Set<Class<?>> expected =
            new HashSet<>( Arrays.asList( MyCollection2.class, MyCollection3.class, MyCollection4.class ) );
        Set<Class<?>> actual = TypeUtils.findImplementations( classPath, prefix, resolved );
        assertEquals( expected, actual );
    }

    private static boolean checkAssignable( TypeToken<?> resolved, Class<?> implClass )
    {
        boolean res = TypeUtils.isAssignable( resolved, implClass );
        System.out.println( "fld = new " + implClass.getSimpleName() + "() -> " + ( res ? "yes" : "no" ) );
        return res;
    }

}