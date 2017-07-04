package org.foo;

import java.lang.reflect.Field;
import java.util.Map;
import java.util.TreeMap;

import com.spr.ajwf.commons.logger.Logger;

public final class TreeTest {
    
    private TreeTest() {}
    
    static final Logger LOGGER = new Logger(TreeTest.class);
    
    private static Integer[] lookupKeys = { -100, -99, -98, 9, 10, 11, 19, 20, 21 };
    
    private static Integer[] expectedFloors = { null, -99, -99, 8, 10, 10, 10, 20, 20 };
    
    private static Integer[] expectedCeils = { -99, -99, -1, 10, 10, 20, 20, 20, null };
    
    private static Integer[] expectedLowers = { null, null, -99, 8, 8, 10, 10, 10, 20 };
    
    private static Integer[] expectedUppers = { -99, -1, -1, 10, 20, 20, 20, null, null };
    
    public static void main(final String[] args) throws Exception {
        final TreeMap<Integer, String> tree = new TreeMap<Integer, String>();
        
        assertNull(tree.floorKey(Integer.MAX_VALUE));
        
        tree.put((10), "ten");
        tree.put((20), "twenty");
        tree.put((-99), "minus ninety-nine");
        tree.put((8), "eight");
        tree.put((-1), "minus one");
        tree.put((0), "zero");
        
        final Integer rootKey = getRootEntry(tree).getKey();
        assertEquals(10, rootKey);
        // System.out.println("root key: " + rootKey);
        
        assertNull(tree.floorKey(-100));
        assertEquals(-99, tree.floorKey(-99));
        assertEquals(-99, tree.floorKey(-98));
        
        assertEquals(8, tree.floorKey(9));
        assertEquals(10, tree.floorKey(10));
        assertEquals(10, tree.floorKey(11));
        
        assertEquals(10, tree.floorKey(19));
        assertEquals(20, tree.floorKey(20));
        assertEquals(20, tree.floorKey(21));
        
        // /////////////
        
        assertEquals(-99, tree.ceilingKey(-100));
        assertEquals(-99, tree.ceilingKey(-99));
        assertEquals(-1, tree.ceilingKey(-98));
        
        assertEquals(10, tree.ceilingKey(9));
        assertEquals(10, tree.ceilingKey(10));
        assertEquals(20, tree.ceilingKey(11));
        
        assertEquals(20, tree.ceilingKey(19));
        assertEquals(20, tree.ceilingKey(20));
        assertNull(tree.ceilingKey(21));
        
        // /////////////
        
        assertNull(tree.lowerKey(-100));
        assertNull(tree.lowerKey(-99));
        assertEquals(-99, tree.lowerKey(-98));
        
        assertEquals(8, tree.lowerKey(9));
        assertEquals(8, tree.lowerKey(10));
        assertEquals(10, tree.lowerKey(11));
        
        assertEquals(10, tree.lowerKey(19));
        assertEquals(10, tree.lowerKey(20));
        assertEquals(20, tree.lowerKey(21));
        
        // /////////////
        
        assertEquals(-99, tree.higherKey(-100));
        assertEquals(-1, tree.higherKey(-99));
        assertEquals(-1, tree.higherKey(-98));
        
        assertEquals(10, tree.higherKey(9));
        assertEquals(20, tree.higherKey(10));
        assertEquals(20, tree.higherKey(11));
        
        assertEquals(20, tree.higherKey(19));
        assertNull(tree.higherKey(20));
        assertNull(tree.higherKey(21));
    }
    
    @SuppressWarnings("unchecked")
    private static <K> Map.Entry<K, ?> getRootEntry(final TreeMap<K, ?> tree) throws Exception {
        final Field rootfield = tree.getClass().getDeclaredField("root");
        rootfield.setAccessible(true);
        return (Map.Entry<K, ?>) rootfield.get(tree);
    }
    
    private static void assertEquals(final Object expected, final Object actual) throws Exception {
        if (expected == actual) {
            return;
        }
        if (expected != null && !expected.equals(actual)) {
            throw new Exception("expected: " + expected + " actual: " + actual);
        }
    }
    
    private static void assertNull(final Object actual) throws Exception {
        assertEquals(null, actual);
    }
}