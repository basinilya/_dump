package typetoken.getsubtype.test;

import java.util.ArrayList;
import java.util.List;

import junit.framework.TestCase;

import com.google.common.reflect.TypeToken;

// https://github.com/google/guava/issues/3048
@SuppressWarnings( { "unchecked", "rawtypes", "serial" } )
public class TestGuava3048
    extends TestCase
{
    private int counter;

    static class LongList
        extends ArrayList<Long>
    {
    }

    static class StringList
        extends ArrayList<String>
    {
    }

    static class StringList2<V extends CharSequence>
        extends ArrayList<V>
    {
    }

    private TypeToken getSubtype( TypeToken resolvedSuperTt, Class<?> subclass )
    {
        try
        {
            TypeToken res = resolvedSuperTt.getSubtype( subclass );
            System.out.println( res + " o" + ( counter++ ) + ";" );
            return res;
        }
        catch ( IllegalArgumentException e )
        {
            return null;
        }
    }

    private TypeToken<List<? extends Number>> type = new TypeToken<List<? extends Number>>()
    {
    };

    public void testGood()
        throws Exception
    {
        assertNotNull( getSubtype( type, LongList.class ) );
    }

    public void testBad()
        throws Exception
    {
        assertNull( getSubtype( type, StringList.class ) );
    }

    public void _testBug()
        throws Exception
    {
        // bad: List<? extends Number> x = new StringList2<Number>();
        assertNull( getSubtype( type, StringList2.class ) );
    }
}
