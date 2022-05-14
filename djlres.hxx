#pragma once

//
// Minimal functions to read and write registry values for application state
//

class CDJLRegistry
{
    public:
        static BOOL createRegistryKey( HKEY hKeyParent, PWCHAR subkey )
        {
            DWORD dwDisposition;
            HKEY  hKey;
            LSTATUS ret = RegCreateKeyEx( hKeyParent, subkey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition );
        
            if ( ERROR_SUCCESS != ret )
                return false;
        
            RegCloseKey( hKey );
            return true;
        } //createRegistryKey
        
        static BOOL writeStringToRegistry( HKEY hKeyParent, PWCHAR subkey, PWCHAR valueName, PWCHAR strData )
        {
            // make sure the key exists so the value can be written
        
            createRegistryKey( HKEY_CURRENT_USER, subkey );
        
            HKEY hKey;
            LSTATUS ret = RegOpenKeyEx( hKeyParent, subkey, 0, KEY_WRITE, &hKey );
        
            if ( ERROR_SUCCESS == ret )
            {
                DWORD bytes = (DWORD) ( sizeof WCHAR * ( wcslen( strData ) + 1 ) );
                ret = RegSetValueEx( hKey, valueName, 0, REG_SZ, (LPBYTE) strData, bytes );
        
                RegCloseKey( hKey );
            }
        
            return ( ERROR_SUCCESS == ret );
        } //writeStringToRegistry
        
        static BOOL readStringFromRegistry( HKEY hKeyParent, PWCHAR subkey, PWCHAR valueName, WCHAR * pwcData, DWORD lenInBytes )
        {
            HKEY hKey;
            LSTATUS ret = RegOpenKeyEx( hKeyParent, subkey, 0, KEY_READ, &hKey );
        
            if ( ERROR_SUCCESS == ret )
            {
                ret = RegQueryValueEx( hKey, valueName, NULL, NULL, (BYTE *) pwcData, &lenInBytes );
        
                RegCloseKey( hKey );
            }
        
            return ( ERROR_SUCCESS == ret );
        } //readStringFromRegistry
};                 
