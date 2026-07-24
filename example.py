# stupid proxy that blocks domains and apps
# run this crap as admin or registry writes will fail, obviously

import winreg
import ctypes
import subprocess
import sys
import os
import time
import struct
import base64
import psutil
import fnmatch
from typing import Optional
from proxy import Proxy
from proxy.http.proxy import HttpProxyBasePlugin
from proxy.http.parser import HttpParser
from proxy.http.exception import HttpRequestRejected

# blocklist domains encoded to base64 so it doesn't look like a mess
B64_BLOCKLIST = "Ki5tY2FmZWUuY29tLCoubWNhZmVlc2VjdXJlLmNvbSxtY3VwZGF0ZS5jb20sKi50cmVsbGl4LmNvbSwqLnRyZW5kbWljcm8uY29tLCoudHJlbmRtaWNyby5jby5qcCxhY3RpdmV1cGRhdGUudHJlbmRtaWNyby5jb20sdG1zcy50cmVuZG1pY3JvLmNvbSwqLmF2aXJhLmNvbSx1cGRhdGUuYXZpcmEuY29tLHBsYXRmb3JtLmF2aXJhLmNvbSwqLnBhbmRhc2VjdXJpdHkuY29tLCouY2xvdWRhdi5wYW5kYXNvZnR3YXJlLmNvbSwqLmYtc2VjdXJlLmNvbSwqLndlYnJvb3QuY29tLCoud2Vicm9vdGFueXdoZXJlLmNvbSwqLmdkYXRhc29mdHdhcmUuY29tLHVwZGF0ZS5nZGF0YS5kZSwqLmNvbW9kby5jb20sKi5kb3dubG9hZC5jb21vZG8uY29tLCouZHJ3ZWIuY29tLHVwZGF0ZS5kcndlYi5jb20sKi5lbXNpc29mdC5jb20sdXBkYXRlcy5lbXNpc29mdC5jb20sKi50b3RhbGF2LmNvbSwqLmNsYW1hdi5uZXQsZGF0YWJhc2UuY2xhbWF2Lm5ldCwqLnpvbmVhbGFybS5jb20sdXBkYXRlLnpvbmVhbGFybS5jb20sKi52aXByZS5jb20sdXBkYXRlcy52aXByZS5jb20sKi5haG5sYWIuY29tLHVwZGF0ZS5haG5sYWIuY29tLCoucXVpY2toZWFsLmNvbSx1cGRhdGUucXVpY2toZWFsLmNvbSwqLms3Y29tcHV0aW5nLmNvbSx1cGRhdGUuazdjb21wdXRpbmcuY29tLCouMzYwLmNuLCouMzYwdG90YWxzZWN1cml0eS5jb20sKi5pb2JpdC5jb20sdXBkYXRlLmlvYml0LmNvbSxzbWFkYXYubmV0LCoucGNtYXRpYy5jb20sKi5lc2NhbmF2LmNvbSx1cGRhdGUuZXNjYW5hdi5jb20sKi50cnVzdHBvcnQuY29tLCouc3VyZnJpZ2h0Lm5sLCouaGl0bWFucHJvLmNvbSwqLmN5bGFuY2UuY29tLCouYmxhY2tiZXJyeS5jb20sKi5zZW50aW5lbG9uZS5uZXQsKi5mb3J0aW5ldC5jb20sKi5mb3J0aWd1YXJkLmNvbSwqLmhlaW1kYWxzZWN1cml0eS5jb20sKi5jeWJlcmVhc29uLmNvbSwqLmZpcmVleWUuY29tLCoua2FzcGVyc2t5LmNvbSwqLmthc3BlcnNreS1sYWJzLmNvbSxkbmwtKi5rYXNwZXJza3kuY29tLCoua2xucy5rYXNwZXJza3ktbGFicy5jb20sKi5nZW8ua2FzcGVyc2t5LmNvbSxzLmthc3BlcnNreS1sYWJzLmNvbSwqLmJpdGRlZmVuZGVyLmNvbSwqLmJpdGRlZmVuZGVyLm5ldCx1cGdyYWRlLmJpdGRlZmVuZGVyLmNvbSxuaW1idXMuYml0ZGVmZW5kZXIubmV0LGNsb3VkLmJpdGRlZmVuZGVyLmNvbSxjZW50cmFsLmJpdGRlZmVuZGVyLmNvbSwqLm1hbHdhcmVieXRlcy5jb20sKi5td2JzeXMuY29tLGNkbi5td2JzeXMuY29tLGtleXN0b25lLm13YnN5cy5jb20sdGVsZW1ldHJ5Lm1hbHdhcmVieXRlcy5jb20saHViYmxlLm1iLWludGVybmFsLmNvbSwqLmVzZXQuY29tLHVwZGF0ZS5lc2V0LmNvbSx1bSouZXNldC5jb20sdHMuZXNldC5jb20sZWR0ZC5lc2V0LmNvbSxwa2kuZXNldC5jb20sKi5hdmFzdC5jb20sKi5hdmcuY29tLHN1LmF2YXN0LmNvbSxmZi5hdmFzdC5jb20sdjdldmVudC5zdGF0cy5hdmFzdC5jb20scG5zLmF2YXN0LmNvbSxjZG4uYXZhc3QuY29tLCoubm9ydG9uLmNvbSwqLnN5bWFudGVjLmNvbSwqLnN5bWFudGVjbGl2ZXVwZGF0ZS5jb20sbGl2ZXVwZGF0ZS5zeW1hbnRlY2xpdmV1cGRhdGUuY29tLGxpdmV1cGRhdGUuc3ltYW50ZWMuY29tLHNoYXN0YS5zeW1hbnRlYy5jb20sc3RhdHMuc3ltYW50ZWMuY29tLGRlZmluaXRpb251cGRhdGVzLm1pY3Jvc29mdC5jb20sZ28ubWljcm9zb2Z0LmNvbSwqLnVwZGF0ZS5taWNyb3NvZnQuY29tLCoud2luZG93c3VwZGF0ZS5jb20sKi5kb3dubG9hZC53aW5kb3dzdXBkYXRlLmNvbSwqLmRlbGl2ZXJ5Lm1wLm1pY3Jvc29mdC5jb20seC5jcC53ZC5taWNyb3NvZnQuY29tLGNkbi54LmNwLndkLm1pY3Jvc29mdC5jb20sd2RjcC5taWNyb3NvZnQuY29tLHdkY3BhbHQubWljcm9zb2Z0LmNvbSx1bml0ZWRzdGF0ZXMueC5jcC53ZC5taWNyb3NvZnQuY29tLCouc21hcnRzY3JlZW4ubWljcm9zb2Z0LmNvbSwqLnNtYXJ0c2NyZWVuLXByb2QubWljcm9zb2Z0LmNvbSwqLnVycy5taWNyb3NvZnQuY29tLCouY2hlY2thcHBleGVjLm1pY3Jvc29mdC5jb20sc2V0dGluZ3Mtd2luLmRhdGEubWljcm9zb2Z0LmNvbSxzZWN1cmUuZXZlbnRzLmRhdGEubWljcm9zb2Z0LmNvbSxyZWZsZWN0b3IuZGVmZW5kZXIubWljcm9zb2Z0LmNvbQ=="

# apps allowed to bypass blocklist
B64_EXCLUSION_PATHS = "QzpcUHJvZ3JhbSBGaWxlc1xHb29nbGVcQ2hyb21lXEFwcGxpY2F0aW9ufEM6XFByb2dyYW0gRmlsZXMgKHg4NilcTWljcm9zb2Z0XEVkZ2VcQXBwbGljYXRpb258QzpcUHJvZ3JhbSBGaWxlc1xNb3ppbGxhIEZpcmVmb3h8QzpcVXNlcnNcPHVzZXI+XEFwcERhdGFcTG9jYWxcUHJvZ3JhbXNcT3BlcmF8QzpcUHJvZ3JhbSBGaWxlc1xaZW4gQnJvd3NlcnxDOlxQcm9ncmFtIEZpbGVzXEJyYXZlU29mdHdhcmVcQnJhdmUtQnJvd3NlclxBcHBsaWNhdGlvbnxDOlxVc2Vyc1w8dXNlcj5cQXBwRGF0YVxMb2NhbFxWaXZhbGRpXEFwcGxpY2F0aW9ufEM6XFByb2dyYW0gRmlsZXNcV2luZG93c0FwcHNcVGhlQnJvd3NlckNvbXBhbnkuQXJjXy4uLnxDOlxVc2Vyc1w8dXNlcj5cQXBwRGF0YVxMb2NhbFxZYW5kZXhcWWFuZGV4QnJvd3NlclxBcHBsaWNhdGlvbnxDOlxQcm9ncmFtIEZpbGVzXFdpbmRvd3NBcHBzXER1Y2tEdWNrR28uLi58QzpcVXNlcnNcPHVzZXI+XERlc2t0b3BcVG9yIEJyb3dzZXJcQnJvd3NlcnxDOlxQcm9ncmFtIEZpbGVzXExpYnJlV29sZnxDOlxQcm9ncmFtIEZpbGVzXEZsb29ycHxDOlxVc2Vyc1w8dXNlcj5cQXBwRGF0YVxMb2NhbFxUaG9yaXVtXEFwcGxpY2F0aW9ufEM6XFByb2dyYW0gRmlsZXNcV2F0ZXJmb3h8QzpcVXNlcnNcPHVzZXI+XEFwcERhdGFcTG9jYWxcRXBpYyBQcml2YWN5IEJyb3dzZXJcQXBwbGljYXRpb258QzpcUHJvZ3JhbSBGaWxlc1xQYWxlIE1vb258QzpcUHJvZ3JhbSBGaWxlcyAoeDg2KVxNYXh0aG9uXEJpbnxDOlxQcm9ncmFtIEZpbGVzICh4ODYpXFVDQnJvd3NlclxBcHBsaWNhdGlvbnxDOlxQcm9ncmFtIEZpbGVzICh4ODYpXFNhZmFyaQ=="

# apps to completely block from using the network
B64_BLOCKLIST_PATHS = "QzpcUHJvZ3JhbURhdGFcTWljcm9zb2Z0XFdpbmRvd3MgRGVmZW5kZXJcUGxhdGZvcm1cfEM6XFByb2dyYW0gRmlsZXNcV2luZG93cyBEZWZlbmRlclx8QzpcUHJvZ3JhbSBGaWxlcyAoeDg2KVxLYXNwZXJza3kgTGFiXEthc3BlcnNreSBBbnRpLVZpcnVzXHxDOlxQcm9ncmFtIEZpbGVzXEJpdGRlZmVuZGVyXEJpdGRlZmVuZGVyIFNlY3VyaXR5XHxDOlxQcm9ncmFtIEZpbGVzXE1hbHdhcmVieXRlc1xBbnRpLU1hbHdhcmVcfEM6XFByb2dyYW0gRmlsZXNcRVNFVFxFU0VUIFNlY3VyaXR5XHxDOlxQcm9ncmFtIEZpbGVzXEF2YXN0IFNvZnR3YXJlXEF2YXN0XHxDOlxQcm9ncmFtIEZpbGVzXEFWR1xBbnRpdmlydXNcfEM6XFByb2dyYW0gRmlsZXNcTm9ydG9uIFNlY3VyaXR5XEVuZ2luZVx8QzpcUHJvZ3JhbSBGaWxlc1xNY0FmZWVcVmlydXNTY2FuIEVudGVycHJpc2VcfEM6XFByb2dyYW0gRmlsZXNcQ29tbW9uIEZpbGVzXE1jQWZlZVxQbGF0Zm9ybVx8QzpcUHJvZ3JhbSBGaWxlc1xUcmVuZCBNaWNyb1xBTVNQXHxDOlxQcm9ncmFtIEZpbGVzICh4ODYpXEF2aXJhXEFudGl2aXJ1c1x8QzpcUHJvZ3JhbSBGaWxlcyAoeDg2KVxQYW5kYSBTZWN1cml0eVxQYW5kYSBTZWN1cml0eSBQcm90ZWN0aW9uXHxDOlxQcm9ncmFtIEZpbGVzICh4ODYpXEYtU2VjdXJlXEFudGktVmlydXNcfEM6XFByb2dyYW0gRmlsZXNcV2Vicm9vdFx8QzpcUHJvZ3JhbSBGaWxlcyAoeDg2KVxHIERBVEFcSW50ZXJuZXRTZWN1cml0eVxBVktcfEM6XFByb2dyYW0gRmlsZXNcQ09NT0RPXENPTU9ETyBJbnRlcm5ldCBTZWN1cml0eVx8QzpcUHJvZ3JhbSBGaWxlc1xEcldlYlx8QzpcUHJvZ3JhbSBGaWxlc1xFbXNpc29mdCBBbnRpLU1hbHdhcmVcfEM6XFByb2dyYW0gRmlsZXNcVG90YWxBVlx8QzpcUHJvZ3JhbSBGaWxlc1xDbGFtQVZcfEM6XFByb2dyYW0gRmlsZXMgKHg4NilcQ2hlY2tQb2ludFxab25lQWxhcm1cfEM6XFByb2dyYW0gRmlsZXMgKHg4NilcVklQUkVcfEM6XFByb2dyYW0gRmlsZXNcQWhuTGFiXFYzSVM5MFx8QzpcUHJvZ3JhbSBGaWxlc1xRdWljayBIZWFsXFF1aWNrIEhlYWwgVG90YWwgU2VjdXJpdHlcfEM6XFByb2dyYW0gRmlsZXMgKHg4NilcSzcgQ29tcHV0aW5nXEs3VFNlY3VyaXR5XHxDOlxQcm9ncmFtIEZpbGVzICh4ODYpXDM2MFxUb3RhbCBTZWN1cml0eVx8QzpcUHJvZ3JhbSBGaWxlcyAoeDg2KVxJT2JpdFxJT2JpdCBNYWx3YXJlIEZpZ2h0ZXJcfEM6XFByb2dyYW0gRmlsZXMgKHg4NilcU21hZGF2XHxDOlxQcm9ncmFtIEZpbGVzXFBDIE1hdGljXHxDOlxQcm9ncmFtIEZpbGVzXGVTY2FuXHxDOlxQcm9ncmFtIEZpbGVzXFRydXN0UG9ydFxUcnVzdFBvcnQgQW50aXZpcnVzXHxDOlxQcm9ncmFtIEZpbGVzXEhpdG1hblByb1x8QzpcUHJvZ3JhbSBGaWxlc1xDeWxhbmNlXERlc2t0b3BcfEM6XFByb2dyYW0gRmlsZXNcU2VudGluZWxPbmVcU2VudGluZWwgQWdlbnRcfEM6XFByb2dyYW0gRmlsZXNcRm9ydGluZXRcRm9ydGlDbGllbnRcfEM6XFByb2dyYW0gRmlsZXMgKHg4NilcSGVpbWRhbFx8QzpcUHJvZ3JhbSBGaWxlc1xDeWJlcmVhc29uIEFjdGl2ZVByb2JlXHxDOlxQcm9ncmFtIEZpbGVzXEZpcmVFeWVceGFndFw="


class AdblockPlugin(HttpProxyBasePlugin):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # parse blocklist and try not to break python doing so
        dec_domains = base64.b64decode(B64_BLOCKLIST).decode('utf-8')
        self.blocked_domains = set([d.strip() for d in dec_domains.split(',') if d.strip()])
        
        # fix users path dynamically cuz windows is dumb and path depends on current user
        ex_paths_str = base64.b64decode(B64_EXCLUSION_PATHS).decode('utf-8')
        username = os.environ.get("USERNAME", "user")
        ex_paths_str = ex_paths_str.replace("<user>", username)
        self.exclusion_paths = [p.strip().lower() for p in ex_paths_str.split('|') if p.strip()]

        # blocklist paths
        blocked_paths_str = base64.b64decode(B64_BLOCKLIST_PATHS).decode('utf-8')
        blocked_paths_str = blocked_paths_str.replace("<user>", username)
        self.blocklist_paths = [p.strip().lower() for p in blocked_paths_str.split('|') if p.strip()]

    def get_client_exe_path(self):
        # caching this because querying connections constantly makes everything lag like hell
        if hasattr(self, '_cached_exe_path'):
            return getattr(self, '_cached_exe_path')

        exe_path = None
        try:
            client_port = None
            if hasattr(self.client, 'addr'):
                client_port = self.client.addr[1]
            elif hasattr(self.client, 'address'):
                client_port = self.client.address[1]

            if client_port:
                # scanning TCP table because windows API lacks simple get-process-by-socket function in python
                for conn in psutil.net_connections(kind='tcp'):
                    if conn.laddr and conn.laddr.port == client_port:
                        if conn.pid:
                            exe_path = psutil.Process(conn.pid).exe().lower()
                        break
        except Exception:
            pass # ignore errors silently, just move on
        
        self._cached_exe_path = exe_path
        return exe_path

    def handle_client_request(self, request: HttpParser) -> Optional[HttpParser]:
        if request.host:
            host_str = request.host.decode('utf-8')

            # block requests from blacklist apps completely
            if self.blocklist_paths:
                exe_path = self.get_client_exe_path()
                if exe_path:
                    for bp in self.blocklist_paths:
                        if exe_path.startswith(bp):
                            print(f"[BLOCKED APP] {exe_path} trying to touch {host_str} -> blocking")
                            raise HttpRequestRejected(
                                status_code=b'403',
                                reason=b'Forbidden',
                                body=b'Blocked app'
                            )
            
            # block specific bad domains
            if any(fnmatch.fnmatch(host_str, bd) or host_str == bd.lstrip('*.') for bd in self.blocked_domains):
                # browsers get a free pass, ignore domain blocks
                exe_path = self.get_client_exe_path()
                if exe_path:
                    for ex_path in self.exclusion_paths:
                        if exe_path.startswith(ex_path):
                            print(f"[WHITELIST CLOUD PASS] {exe_path} accessing {host_str} -> letting it slide")
                            return request
                
                print(f"[BLOCKED DOMAIN] domain blacklist hit -> {host_str}")
                raise HttpRequestRejected(
                    status_code=b'403',
                    reason=b'Forbidden',
                    body=b'Blocked domain'
                )
        return request


# useless windows registry hacks and uac bypass logic
def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except Exception:
        return False


def run_as_admin():
    # reinvoking ourselves with admin because windows registry permissions are a joke
    params = ' '.join([f'"{arg}"' for arg in sys.argv])
    ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, params, None, 1)
    sys.exit(0)


def set_hkcu_proxy(enable, server):
    try:
        reg_key = winreg.OpenKey(
            winreg.HKEY_CURRENT_USER,
            r'Software\Microsoft\Windows\CurrentVersion\Internet Settings',
            0,
            winreg.KEY_ALL_ACCESS
        )
        if enable:
            winreg.SetValueEx(reg_key, 'ProxyEnable', 0, winreg.REG_DWORD, 1)
            winreg.SetValueEx(reg_key, 'ProxyServer', 0, winreg.REG_SZ, server)
        else:
            winreg.SetValueEx(reg_key, 'ProxyEnable', 0, winreg.REG_DWORD, 0)
        winreg.CloseKey(reg_key)
    except Exception:
        pass


def _build_default_connection_settings(proxy_enabled, proxy_server=""):
    # assembling this ugly binary struct because internet explorer defaults are weird
    version = 0x46
    counter = 0x01
    flags = 0x03 if proxy_enabled else 0x01

    proxy_bytes = proxy_server.encode('ascii') if proxy_server else b''
    bypass_bytes = b''
    autoconfig_bytes = b''

    data = struct.pack('<III', version, counter, flags)
    data += struct.pack('<I', len(proxy_bytes)) + proxy_bytes
    data += struct.pack('<I', len(bypass_bytes)) + bypass_bytes
    data += struct.pack('<I', len(autoconfig_bytes)) + autoconfig_bytes
    data += b'\x00' * 32
    return data


def set_hklm_proxy(enable, server):
    try:
        reg_key = winreg.OpenKey(
            winreg.HKEY_LOCAL_MACHINE,
            r'SOFTWARE\Microsoft\Windows\CurrentVersion\Internet Settings\Connections',
            0,
            winreg.KEY_ALL_ACCESS
        )
        data = _build_default_connection_settings(enable, server if enable else "")
        winreg.SetValueEx(reg_key, 'DefaultConnectionSettings', 0, winreg.REG_BINARY, data)
        winreg.CloseKey(reg_key)
    except Exception:
        pass


def set_winhttp_proxy(enable, server):
    try:
        # system-wide service proxy setup, netsh logic is so annoying
        if enable:
            cmd = f'netsh winhttp set proxy proxy-server="{server}" bypass-list="localhost;127.0.0.1"'
            print(f"Setting proxy: {cmd}")
            subprocess.run(cmd, shell=True, capture_output=True)
        else:
            subprocess.run('netsh winhttp reset proxy', shell=True, capture_output=True)
    except Exception:
        pass


def notify_system_proxy_change():
    # notify system so we don't have to reboot this toaster
    internet_set_option = ctypes.windll.wininet.InternetSetOptionW
    internet_set_option(0, 39, 0, 0) # settings changed option
    internet_set_option(0, 37, 0, 0) # refresh option


def enable_all_proxies(server):
    set_hkcu_proxy(True, server)
    set_hklm_proxy(True, server)
    set_winhttp_proxy(True, server)
    notify_system_proxy_change()


def disable_all_proxies():
    set_hkcu_proxy(False, "")
    set_hklm_proxy(False, "")
    set_winhttp_proxy(False, "")
    notify_system_proxy_change()


def main():
    if not is_admin():
        run_as_admin()
        return

    HOST = '127.0.0.1'
    PORT = 8899
    SERVER = f"{HOST}:{PORT}"

    enable_all_proxies(SERVER)

    try:
        # spinning up proxy.py module, let's hope it doesn't crash on start
        with Proxy(['--hostname', HOST, '--port', str(PORT), '--plugins', '__main__.AdblockPlugin']):
            while True:
                time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        disable_all_proxies()


if __name__ == '__main__':
    main()
