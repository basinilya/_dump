package com.common.jsp.beans;

import java.lang.reflect.Field;
import java.lang.reflect.GenericDeclaration;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.lang.reflect.TypeVariable;
import java.sql.Timestamp;
import java.util.*;

import junit.framework.TestCase;
import static com.common.jsp.beans.TypeUtils.checkAssignable;

import com.google.common.reflect.TypeToken;

@SuppressWarnings( { "serial", "rawtypes" } )
public class TestSimple
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

    public static class MyCollection5<T extends Number>
        extends ArrayList<T>
    {
    }

    public static class MyCollection6<T>
        extends ArrayList<Number>
    {
    }

    public static Collection<Timestamp> fld;

    @SuppressWarnings( "unchecked" )
    private void thisCompiles()
    {
        fld = new ArrayList<>();
        // bad: fld = new MyCollection1();
        fld = new MyCollection2();
        fld = new MyCollection3<>();
        fld = new MyCollection4(); // just a warning
        // bad: fld = new MyCollection5<>();
        // bad: fld = new MyCollection6<>();
    }

    public void testSomple()
        throws Exception
    {
        thisCompiles();

        Field getter = TestSimple.class.getField( "fld" );
        Type memberType = getter.getGenericType();

        TypeToken<?> resolveContext = TypeToken.of( TestSimple.class );
        @SuppressWarnings( "unchecked" )
        TypeToken<Collection> resolved = (TypeToken<Collection>) resolveContext.resolveType( memberType );
        // System.out.println( "type of fld: " + resolved );

        assertTrue( checkAssignable( resolved, ArrayList.class ) );

        assertFalse( checkAssignable( resolved, MyCollection1.class ) );
        assertTrue( checkAssignable( resolved, MyCollection2.class ) );
        assertTrue( checkAssignable( resolved, MyCollection3.class ) );
        assertTrue( checkAssignable( resolved, MyCollection4.class ) );
        assertFalse( checkAssignable( resolved, MyCollection5.class ) );
        assertFalse( checkAssignable( resolved, MyCollection6.class ) );
    }

}
