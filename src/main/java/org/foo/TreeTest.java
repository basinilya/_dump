package org.foo;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

public final class TreeTest extends TreeMap<Integer, Integer> {
    
    private TreeTest() {}
    
    public static void main(final String[] args) throws Exception {
        new TreeTest().printTestLines();
    }
    
    private void printTestLine(final String func, final String key, final Integer got) {
        System.out.println((got == null ? "assertNull(" : ("assertEquals(" + got + ", ")) + func
                + "(" + key + "));");
    }
    
    private void printTestLine(final String func, final Integer key, final Integer got) {
        printTestLine(func, key.toString(), got);
    }
    
    private void printTestLines() throws Exception {
        
        Set<Map.Entry<Integer, Integer>> setView;
        final HashMap<Integer, Integer> hm = new HashMap<Integer, Integer>();
        final SimpleEntry<Integer, Integer> entry = new SimpleEntry<Integer, Integer>(1, 1);
        setView = hm.entrySet();
        setView.remove(entry);
        
        int i;
        
        for (i = -7; i <= 7; i += 2) {
            put(i, i);
        }
        // SimpleEntry;
        setView = this.entrySet();
        setView.remove(entry);
        setView.add(new SimpleEntry<Integer, Integer>(1, 1));
        
        printTestLine("firstKey", "", firstKey());
        printTestLine("lastKey", "", lastKey());
        
        int max;
        for (i = firstKey() - 1, max = lastKey() + 1; i <= max; i++) {
            printTestLine("get", i, get(i));
            printTestLine("floorKey", i, floorKey(i));
            printTestLine("ceilingKey", i, ceilingKey(i));
            printTestLine("lowerKey", i, lowerKey(i));
            printTestLine("higherKey", i, higherKey(i));
        }
        
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