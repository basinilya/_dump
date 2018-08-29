package com.common.jsp.beans;

import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

public class Editor
{
    private Object rootObject;

    private ArrayList<String> pathEntryNames = new ArrayList<>();
    private ArrayList<Object> pathEntries = new ArrayList<>();

    public ArrayList<Object> getPathEntries()
    {
        return pathEntries;
    }

    public void setPathEntries( ArrayList<Object> pathEntries )
    {
        this.pathEntries = pathEntries;
    }

    public ArrayList<String> getPathEntryNames()
    {
        return pathEntryNames;
    }

    public void setPathEntryNames( ArrayList<String> pathEntries )
    {
        this.pathEntryNames = pathEntries;
    }

    public Object getLeafObject()
    {
        return null;
    }

    public Object getRootObject()
    {
        return rootObject;
    }

    public void setRootObject( Object rootPojo )
    {
        LOGGER.log( Level.INFO, "> setRootObject {0}" , new Object[] { rootPojo } );
        this.rootObject = rootPojo;
    }

    public void addPathEntry( String entry )
    {
        LOGGER.log( Level.INFO, "> addPathEntry {0}" , new Object[] { entry } );
        pathEntryNames.add( entry );
        ArrayList l = null;
        l.get(8);
    }

    private static final Logger LOGGER = Logger.getLogger( Editor.class.getName() );
}
