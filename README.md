<a id="readme-top"></a>

[![Stars][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![License][license-shield]][license-url]
![PoC](https://img.shields.io/badge/type-PoC%20only-critical?style=for-the-badge)
![Platform](https://img.shields.io/badge/platform-Windows-blue?style=for-the-badge)

<br />
<div align="center">
  <h1>AVNetBlind</h1>
  <p><strong>[ PROOF OF CONCEPT ] Dual-Layer Antivirus Network Isolation via User-Mode APIs</strong></p>
  <p>
    A research PoC(Proof of Concept) that demonstrates how to isolate antivirus software from its update/telemetry/cloud by utilizing just user-mode Win32 API’s. No kernel driver, no exploit, no patch.
  </p>
  <br/>
  <blockquote>
  <strong>In order to robustly detect any modern AV, a kernel-mode driver (such as an NDIS filter or Minifilter driver) is needed.</strong>
  Without kernel-level access, AV networks stack would not be filtered and low-level network connections could not be managed by the hooks.
  See <a href="#why-this-fails">Why This Fails</a>.
  </blockquote>
</div>

---

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#about">About</a></li>
    <li><a href="#architecture">Architecture</a></li>
    <li><a href="#how-it-works">How It Works - C++ WFP Engine</a></li>
    <li><a href="#reference-impl">Reference Implementation (Python)</a></li>
    <li><a href="#why-this-fails">Why This Fails (PoC Limitations)</a></li>
    <li><a href="#project-structure">Project Structure</a></li>
    <li><a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#build">Build</a></li>
      </ul>
    </li>
    <li><a href="#blocklist">Blocklist</a></li>
    <li><a href="#detection">Detection & VT Score</a></li>
    <li><a href="#disclaimer">Disclaimer</a></li>
  </ol>
</details>

---

## About

<a id="about"></a>

**AVNetBlind** is a PoC that analyzes if AV products can be isolated from the network by using **only administrator-level user-mode Windows APIs**. It focuses on the update, telemetry and cloud-scanning endpoints of 30+ AVs without kernel, no memory patch or exploit.

The project has two components:

| Component | Role |
|---|---|
| `main.cpp` / `main.exe` | **Main Implementation** - native C++ WFP-based network filter (headless background service)|
| `example.py` | **Reference prototype only** - Python HTTP proxy reflecting same logic to a human- readable form. This is NOT the actual product|

> The C++ binary requires administrator privileges. This is enforced by an embedded `app.manifest` with `requireAdministrator` - Windows refuses to launch it without an admin token. **No UAC elevation prompt is triggered.**

---

## Architecture

<a id="architecture"></a>

```
┌──────────────────────────────────────────────────┐
│                    USER MODE                     │
│                                                  │
│          ┌───────────────────────────┐           │
│          │       main.exe (C++)      │           │
│          │   WFP Engine (fwpuclnt)   │           │
│          └──────────────┬────────────┘           │
│                         │                        │
│            ┌────────────┴────────────┐           │
│            ▼                         ▼           │
│     App Identity Filters        IP Block Filters │
│  FWPM_CONDITION_ALE_APP_ID   FWPM_CONDITION_     │
│  AUTH_CONNECT_V4/V6          IP_REMOTE_ADDRESS   │
│            │                         │           │
│            └────────────┬────────────┘           │
│                         ▼                        │
│                  Block / Permit                  │
└──────────────────────────────────────────────────┘
                          │
                   KERNEL MODE
          ┌───────────────────────────┐
          │  AV minifilter / NDIS     │  ← out of reach
          │  PPL-protected processes  │  ← out of reach
          └───────────────────────────┘
```

---

## How It Works - C++ WFP Engine

<a id="how-it-works"></a>

`main.cpp` is a headless Windows GUI process (`/SUBSYSTEM:WINDOWS`) that uses the **Windows Filtering Platform** directly to install network block rules at the kernel ALE callout boundary.

### Evasion-oriented design

`Fwpuclnt.dll` gets loaded dynamically - none of the WFP functions seem to be exported in the import table. The DLL name and the functions names themselves are Base64 encoded:

```cpp
hFwpDll = LoadLibraryA(Base64Decode("ZndwdWNsbnQuZGxs").c_str()); // fwpuclnt.dll
pFwpmFilterAdd0 = (FwpmFilterAdd0_t)GetProcAddress(hFwpDll, Base64Decode("RndwbUZpbHRlckFkZDA=").c_str());
```

AV endpoint domains, executable paths also encoded Base64-encoded - there are not plain strings at all inside of the binary

### Sublayer & filter priority

Custom WFP sublayer installed at weight `0xFFFF` (top priority) install two filter types:

**1. App identity filters** - `FWPM_LAYER_ALE_AUTH_CONNECT_V4/V6` + `FWPM_CONDITION_ALE_APP_ID`:
- Known AV executable directories → `FWP_ACTION_BLOCK` (weight 15)
- Known browser executables → `FWP_ACTION_PERMIT` (weight 14, overrides domain blocks for browsers)

**2. IP address filters** - `FWPM_CONDITION_IP_REMOTE_ADDRESS`:
A background thread resolves each blocked domain via `getaddrinfo` every 5 minutes, deduplicates the IPs, clears old filters, and installs fresh `/32` `FWP_ACTION_BLOCK` rules.

### No persistent state

The WFP session is opened with `FWPM_SESSION_FLAG_DYNAMIC` - all filters are automatically removed when the process exits. A hidden message-loop window (`CreateWindowEx`, not visible) keeps the process alive and provides a clean `WM_DESTROY` shutdown path.

### Admin requirement

`app.manifest` sets `requestedExecutionLevel` to `requireAdministrator`. The OS enforces this at launch - the process token either already carries admin rights or the launch is rejected outright.

---

## Reference Implementation (Python)

<a id="reference-impl"></a>

> **`example.py`** The python script example.py is a high level prototype meant to ease the reading and exploration of the filter logic, but it's not the heart of this PoC, as it relies on third party libraries (proxy.py, psutil) that the C++ version entirely avoids.

`example.py` demonstrates the same idea but via an HTTP proxy:

- Starts `proxy.py` on `127.0.0.1:8899`
- Writes proxy address to `HKCU`, `HKLM`, and `WinHTTP` so system services will use it
- `AdblockPlugin` per-request logic:
  - Terminates requesting process: tcp port PID.exe path via psutil
  - AV processes → `403 Blocked app`
  - AV domains (wildcard `fnmatch`) → `403 Blocked domain`
  - Whitelisted browsers → pass-through

`reset_proxy.py` - when run if it is hung and exits without removing the proxy settings it should remove everything if system has lost internet.

---

## Why This Fails

<a id="why-this-fails"></a>

> **TL;DR: User-mode filtering cannot stop kernel-mode AV network stacks.**

| Limitation | Explanation |
|---|---|
| **HTTP proxy scope** | It only can see HTTP/HTTPS via system proxy from the Python layer. AV clients have own TLS stack and use raw socket - ignores proxy settings. |
| **WFP ALE layer** |  The `FWPM_LAYER_ALE_AUTH_CONNECT` event is triggered when a user-mode socket is created. Traffic is sent below this layer by AV kernel minifilters and NDIS drivers.|
| **Protected Process Light** | PPL processes like the ones from Windows Defender and others are immune to network I/O filtering in the WFP with non-privileged provider callout matching.|
| **AV owns WFP too** | Enterprise AVs can register their own higher-priority WFP providers. They can remove foreign filters or add a `FWP_ACTION_PERMIT` to outrank a block.|
| **CDN IP rotation** | IP-block solution resolves A records on execution. CDN-supported AV infra changes IPs every so often - any address that resolves after the last update cycle is open.|
| **Dynamic session** | With `FWPM_SESSION_FLAG_DYNAMIC` every filter vanishes instantly when the process died. It’s got no persistence, and definitely no resilience. |

**There’s nothing that this tool can do to significantly interfere with any of today’s modern AV products in real world usage without kernel-driver access.**

---

## Project Structure

<a id="project-structure"></a>

```
AVNetBlind/
├── main.cpp           # ★ Initial PoC - WFP engine with C++(no console)headless
├── example.py         #   Reference prototype only-python http proxy eqval
├── reset_proxy.py     #   Utility - system proxy registry state reset
├── resource.rc        #   Win32 resources (version info, icon, manifest)
├── app.manifest       #   Admin - no UAC, admin not forced
├── icon.ico           #   Application icon
└── build.bat          #   MSVC build (cl.exe + rc.exe, x64)
```

---

## Getting Started

<a id="getting-started"></a>

### Prerequisites

<a id="prerequisites"></a>

- Windows 10/11 (x64)
- Visual Studio 2022 - "Desktop development with C++" workload
- Windows SDK with WFP headers (`fwpmu.h`, `fwpmtypes.h`)
- Must be run as **administrator** (manifest enforces this)

---

### Build

<a id="build"></a>

```bat
build.bat
```

Calls `vcvarsall.bat x64`, compiles the resource file, then links:

```bat
cl main.cpp resource.res ws2_32.lib iphlpapi.lib shlwapi.lib wininet.lib advapi32.lib user32.lib shell32.lib /O2 /EHsc
```

The resulting `main.exe` has the manifest embedded. Launch it - if not already running as admin, Windows will reject the launch.

---

## Blocklist

<a id="blocklist"></a>

Update, telemetry, and cloud endpoints for **30+ vendors**:

`McAfee / Trellix` · `Trend Micro` · `Avira` · `Kaspersky` · `Bitdefender` · `Malwarebytes` · `ESET` · `Avast` · `AVG` · `Norton / Symantec` · `Windows Defender` · `SentinelOne` · `Cylance (Blackberry)` · `FireEye` · `Fortinet` · `Heimdal` · `Cybereason` · `Webroot` · `Comodo` · `DrWeb` · `Emsisoft` · `ZoneAlarm` · `VIPRE` · `AhnLab` · `Quick Heal` · `K7` · `360 Total Security` · `IObit` · `Panda` · `F-Secure`

All domain and path lists are Base64-encoded in source  **no plaintext strings** in the compiled binary.

---

## Detection & VT Score

<a id="detection"></a>

`main.exe` - **2 / 70** on VirusTotal.

Contributing factors:
- No WFP funcs in the import table (LoadLibrary + GetProcAddress at runtime)
- no av plaintext domain/path strings
- /SUBSYSTEM:WINDOWS (and a hidden message loop that looks exactly like any normal background GUI app)
- Zero persistent state (FWPM_SESSION_FLAG_DYNAMIC)

---

## Disclaimer

<a id="disclaimer"></a>

> This project is published **for educational and research purposes only.**
>
> This is a proof-of-concept for a type of technique. This is NOT capable of actually breaking any real antivirus product. Do NOT attempt to run this on a machine that you don't own or have prior written approval to test on.
>
> The author accepts no responsibility for the usage of this information.

---

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- MARKDOWN LINKS -->
[stars-shield]: https://img.shields.io/github/stars/yamanist0/AVNetBlind.svg?style=for-the-badge
[stars-url]: https://github.com/yamanist0/AVNetBlind/stargazers
[issues-shield]: https://img.shields.io/github/issues/yamanist0/AVNetBlind.svg?style=for-the-badge
[issues-url]: https://github.com/yamanist0/AVNetBlind/issues
[license-shield]: [https://img.shields.io/github/license/esa/torchquad]
[license-url]: https://github.com/yamanist0/AVNetBlind/blob/master/LICENSE

<p align="center">
  <small>Made with 🤍 by <a href="https://github.com/yamanist0">yamanist</a></small>
</p>
