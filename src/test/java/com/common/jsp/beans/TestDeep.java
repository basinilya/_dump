package com.common.jsp.beans;

import java.lang.reflect.Field;
import java.lang.reflect.Type;

import com.google.common.reflect.TypeToken;

import junit.framework.TestCase;

public class TestDeep extends TestCase {

	public void testDeep() throws Exception {
		Ccc ccc = new Ccc();
		ccc.bbb = new Bbb<Integer>();
		ccc.bbb.aaa = new Aaa<Integer>();

		
        Field member = Ccc.class.getField( "bbb" );
        Type memberType = member.getGenericType();

        TypeToken<?> resolved = TypeToken.of( Ccc.class ).resolveType( memberType );
        System.out.println( "type of fld: " + resolved );		
        
        member = Bbb.class.getField( "aaa" );
        memberType = member.getGenericType();

        resolved = resolved.resolveType( memberType );
        System.out.println( "type of fld: " + resolved );		

        member = Aaa.class.getField( "z" );
        memberType = member.getGenericType();

        resolved = resolved.resolveType( memberType );
        System.out.println( "type of fld: " + resolved );		

        
	}
}
class Aaa<Z> {
	public Z z;
}
class Bbb<Y> {
	public Aaa<Y> aaa;
}
class Ccc {
	public Bbb<Integer> bbb;
}
