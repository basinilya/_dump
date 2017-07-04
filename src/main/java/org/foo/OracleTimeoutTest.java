package org.foo;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.Statement;

public class OracleTimeoutTest {
    
    public static void main(final String[] args) throws Exception {
        Class.forName("oracle.jdbc.OracleDriver");
        final Connection conn = DriverManager.getConnection(
            "jdbc:oracle:thin:@eiger.reksoft.ru:1522:PPB", "ppbbig", "ppbbig");
        final Statement st = conn.createStatement();
        st.setQueryTimeout(1);
        final ResultSet rs = st.executeQuery("select count(*) from cdrcall");
        final int c = rs.getInt(1);
        System.out.println(c);
    }
    
}
