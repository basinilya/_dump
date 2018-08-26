package typetoken.getsubtype.test;

import java.util.ArrayList;
import java.util.List;

import junit.framework.TestCase;

import com.google.common.reflect.TypeToken;

@SuppressWarnings( { "unchecked", "rawtypes" } )
public class TestGetSubtyp2
    extends TestCase
{
    private int counter;

    static class StringList extends ArrayList<String> {}
    static class StringList2<V extends String> extends ArrayList<V> {}
    static class StringList3 extends StringList2<String> {}

    private TypeToken getSubtype(TypeToken resolvedSuperTt, Class<?> subclass) {
        try {
            TypeToken res = resolvedSuperTt.getSubtype( subclass );
            System.out.println(res + " o" + (counter++) + ";");
            return res;
        } catch (IllegalArgumentException e) {
            return null;
        }
    }
    
    public void testGetSubtype()
        throws Exception
    {
        TypeToken<List<? extends Number>> type = new TypeToken<List<? extends Number>>() {};
        
        System.out.println( StringList3.class.getGenericSuperclass() );
        
        // bad: List<? extends Number> x = new StringList2<Number>();
        assertNull(getSubtype(type, StringList.class));
        assertNull(getSubtype(type, StringList2.class));
    }
    
}
