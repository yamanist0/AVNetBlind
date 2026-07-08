# run this if proxy crashed and you cant go online
# resets all proxy settings back to normal

import winreg
import ctypes
import subprocess
import sys

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except Exception:
        return False

def run_as_admin():
    params = ' '.join([f'"{arg}"' for arg in sys.argv])
    ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, params, None, 1)
    sys.exit(0)

def reset_hkcu():
    try:
        reg_key = winreg.OpenKey(
            winreg.HKEY_CURRENT_USER,
            r'Software\Microsoft\Windows\CurrentVersion\Internet Settings',
            0,
            winreg.KEY_ALL_ACCESS
        )
        winreg.SetValueEx(reg_key, 'ProxyEnable', 0, winreg.REG_DWORD, 0)
        winreg.CloseKey(reg_key)
        print("[OK] HKCU proxy disabled")
    except Exception as e:
        print(f"[FAIL] HKCU: {e}")

def reset_hklm():
    try:
        reg_key = winreg.OpenKey(
            winreg.HKEY_LOCAL_MACHINE,
            r'SOFTWARE\Microsoft\Windows\CurrentVersion\Internet Settings\Connections',
            0,
            winreg.KEY_ALL_ACCESS
        )
        # write a clean DefaultConnectionSettings with proxy off
        import struct
        data = struct.pack('<III', 0x46, 0x01, 0x01) # version, counter, flags=direct
        data += struct.pack('<I', 0) + b''  # proxy (empty)
        data += struct.pack('<I', 0) + b''  # bypass (empty)
        data += struct.pack('<I', 0) + b''  # autoconfig (empty)
        data += b'\x00' * 32
        winreg.SetValueEx(reg_key, 'DefaultConnectionSettings', 0, winreg.REG_BINARY, data)
        winreg.CloseKey(reg_key)
        print("[OK] HKLM DefaultConnectionSettings reset")
    except Exception as e:
        print(f"[FAIL] HKLM: {e}")

def reset_winhttp():
    try:
        subprocess.run('netsh winhttp reset proxy', shell=True, capture_output=True)
        print("[OK] WinHTTP proxy reset")
    except Exception as e:
        print(f"[FAIL] WinHTTP: {e}")

def notify_system():
    internet_set_option = ctypes.windll.wininet.InternetSetOptionW
    internet_set_option(0, 39, 0, 0)
    internet_set_option(0, 37, 0, 0)

if __name__ == '__main__':
    if not is_admin():
        run_as_admin()
    else:
        print("resetting all proxy settings...")
        reset_hkcu()
        reset_hklm()
        reset_winhttp()
        notify_system()
        print("\ndone. internet should work now")
        input("press enter to close...")
