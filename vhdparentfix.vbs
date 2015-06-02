Const DeviceType_HardDisk = 3
Const AccessMode_ReadOnly = 0

sub fixdate(sPathParent, sPathChild)
    Dim oExec
    Dim fixargs
    Dim s
    fixargs = "--fix-mtime"
    'fixargs = "--fix-header"
    'fixargs = ""
    Set oExec = WshShell.Exec("vhdstamp.exe """ & sPathParent & """ """ & sPathChild & """ " & fixargs)
    'stdout.Write oExec.StdOut.ReadAll()
    'stdout.WriteLine ""
    s = oExec.StdErr.ReadAll()
    if s <> "" then
        stdout.Write s
        stdout.WriteLine ""
    end if
end sub

sub fixParent(sPathChild)
    Dim VHDChild
    Dim VHDParent
    Dim sPathActualParent
    Dim sPathCurrentParent

    stdout.WriteLine "child         : " & sPathChild
        
    set VHDChild = vb.openMedium(sPathChild, DeviceType_HardDisk, ccessMode_ReadOnly, false)
    set VHDParent = VHDChild.parent
    
    if IsNull(VHDParent) or VHDParent Is Nothing then
        stdout.WriteLine sPathChild & " has no parent"
        exit sub
    end if

    sPathActualParent = VHDParent.location
    stdout.WriteLine "actual parent : " & sPathActualParent

    fixdate sPathActualParent, sPathChild

    sPathCurrentParent = null

    'on error resume next
    set VHDChild = vpc.GetHardDisk(sPathChild)
    ErrNumber = Err.number
    On Error GoTo 0

    if ErrNumber = 0 then
        sPathCurrentParent = VHDChild.Parent.File
        stdout.WriteLine "current parent: " & sPathCurrentParent

        if 0 = StrComp(replace(sPathCurrentParent, "/", "\"), replace(sPathActualParent, "/", "\"), vbTextCompare) then
            stdout.WriteLine "parent ok"
            exit sub
        end if
    else
        stdout.WriteLine "current parent unknown"
    end if

    stdout.WriteLine "fixing parent"
    
    set VHDParent = vpc.GetHardDisk(sPathActualParent)
    
    VHDChild.Parent = VHDParent
    
end sub

sub changeParent(sPathChild, sPathActualParent)
    Dim VHDChild
    Dim VHDParent
    Dim sPathCurrentParent

    stdout.WriteLine "child         : " & sPathChild
    stdout.WriteLine "actual parent : " & sPathActualParent

    fixdate sPathActualParent, sPathChild

    set VHDChild = vpc.GetHardDisk(sPathChild)
    
    sPathCurrentParent = VHDChild.Parent.File
    stdout.WriteLine "current parent: " & sPathCurrentParent
    
    if 0 = StrComp(replace(sPathCurrentParent, "/", "\"), replace(sPathActualParent, "/", "\"), vbTextCompare) then
        stdout.WriteLine "parent ok"
        exit sub
    end if
    
    stdout.WriteLine "fixing parent"
    
    set VHDParent = vpc.GetHardDisk(sPathActualParent)
    
    VHDChild.Parent = VHDParent
    
end sub

Set fso = CreateObject("Scripting.FileSystemObject")
set vb = CreateObject("VirtualBox.VirtualBox")
set vpc = CreateObject("VirtualPC.Application")
set WshShell = WScript.CreateObject("WScript.Shell")

Set stdout = WScript.StdOut
Set stderr = WScript.StdErr

Set arguments = WScript.Arguments

sPathChild = fso.GetAbsolutePathName(arguments(0))

if arguments.Length = 2 then
    sPathActualParent = fso.GetAbsolutePathName(arguments(1))
    changeParent sPathChild, sPathActualParent
else
    fixParent sPathChild
end if

stdout.WriteLine ""
