package com.common.jsp.beans;
import java.lang.reflect.Field;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.Collection;

import com.google.common.reflect.TypeToken;

public class TestTypeInf
{
    public static class MyCollection1 extends ArrayList<Integer> {
    }
    public static class MyCollection2 extends ArrayList<Timestamp> {
    }
    public static class MyCollection3<T> extends ArrayList<T> {
    }
    public static class MyCollection4 extends ArrayList {
    }

    public static Collection<Timestamp> fld;

    public static void main( String[] args )
                    throws Exception
    {
        // bad: fld = new MyCollection1();
        fld = new MyCollection2();
        fld = new MyCollection3(); // just a warning
        fld = new MyCollection4(); // just a warning

        Field getter = TestTypeInf.class.getField( "fld" );
        Type memberType = getter.getGenericType();

        TypeToken<?> resolved = TypeToken.of( TestTypeInf.class ).resolveType( memberType );
        System.out.println( "type of fld: " + resolved );

        checkAssignable(resolved, ArrayList.class);
        checkAssignable(resolved, MyCollection1.class);
        checkAssignable(resolved, MyCollection2.class);
        checkAssignable(resolved, MyCollection3.class); // This should be totally valid
        checkAssignable(resolved, MyCollection4.class); // why no?
    }

    private static void checkAssignable(TypeToken<?> resolved, Class<?> implClass) {
        System.out.println( "fld = new " + implClass.getSimpleName() + "() ->" + (isAssignable( resolved, implClass ) ? "yes" : "no") );
    }


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
}