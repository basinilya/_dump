package typetoken.getsubtype.test;

import java.sql.Timestamp;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import junit.framework.TestCase;

import com.google.common.reflect.TypeToken;

@SuppressWarnings( { "unchecked", "rawtypes" } )
public class TestGetSubtype
    extends TestCase
{
    private int counter;
    
    private TypeToken getSubtype(TypeToken resolvedSuperTt, Class<?> subclass) {
        try {
            TypeToken res = resolvedSuperTt.getSubtype( subclass );
            if (!res.isSubtypeOf( resolvedSuperTt )) {
                // https://github.com/google/guava/issues/3048
                return null;
            }
            System.out.println(res + " o" + (counter++) + ";");
            return res;
        } catch (IllegalArgumentException e) {
            return null;
        }
    }
    
    public void testGetSubtype()
        throws Exception
    {
        TypeToken resolvedMemberTt = new TypeToken<Map<Integer, Collection<Timestamp>>>() {};

        System.out.println("public " + resolvedMemberTt + " map;");

        assertNotNull( getSubtype( resolvedMemberTt, HashMap.class ) );
        assertNotNull( getSubtype( resolvedMemberTt, HashMap_Integer_V.class ) );
        assertNotNull( getSubtype( resolvedMemberTt, HashMap_Integer_Timestamps.class ) );
        assertNotNull( getSubtype( resolvedMemberTt, HashMap_Integer_VTimestamps.class ) );
        assertNull( getSubtype( resolvedMemberTt, HashMap_Raw.class ) );
        assertNull( getSubtype( resolvedMemberTt, HashMap_Integer_Number.class ) );

        // https://github.com/google/guava/issues/3048
        //assertNull( getSubtype( resolvedMemberTt, HashMap_Integer_VNumber.class ) );
    }
}
