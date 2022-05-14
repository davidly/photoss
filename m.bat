del *.obj
del photoss.exe
del photoss.pdb
rc photoss.rc
cl /nologo photoss.cxx /Ox /Qpar /O2 /Oi /Ob2 /EHac /Zi /Gy /D_AMD64_ /link ntdll.lib user32.lib gdi32.lib photoss.res /OPT:REF /subsystem:windows


