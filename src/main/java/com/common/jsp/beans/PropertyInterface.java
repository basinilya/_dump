package com.common.jsp.beans;

/**
 * Maybe override inside JSP for e.g. use Unified EL.
 */
public class PropertyInterface
{
    public Object get( Object parent, Object property )
        throws Exception
    {
        return null;
    }

    public void set( Object parent, Object property, Object value )
        throws Exception
    {
    }

    public void unset( Object parent, Object property )
    {
    }
}
