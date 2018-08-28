package com.common.jsp.beans;

import java.util.ArrayList;

public class Editor
{
    private Object rootObject;

    private ArrayList<String> pathEntries = new ArrayList<>();

    public Object getLeafObject() {
        return null;
    }
    
    public Object getRootObject()
    {
        return rootObject;
    }

    public void setRootObject( Object rootPojo )
    {
        this.rootObject = rootPojo;
    }

    public void addPathEntry( String entry )
    {
        pathEntries.add( entry );
    }
}
