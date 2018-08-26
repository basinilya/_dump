package typetoken.getsubtype.test;

import java.sql.Timestamp;
import java.util.Collection;
import java.util.HashMap;

@SuppressWarnings( "serial" )
public class HashMap_Integer_VTimestamps<V extends Collection<Timestamp>> extends HashMap<Integer, V> {
}