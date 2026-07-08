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
  ⚠️ <strong>To reliably isolate modern AVs, a kernel-mode driver (e.g., NDIS filter or Minifilter driver) is required.</strong>
  Without kernel-mode privileges, user-mode hooks cannot intercept AV network stacks or handle low-level connections.
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
    <li><a href="#how-it-works">How It Works — C++ WFP Engine</a></li>
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
| `main.cpp` / `main.exe` | **Primary implementation** — native C++ WFP-based network filter, headless, runs as a background service |
| `example.py` | **Reference prototype only** — a Python HTTP proxy that models the same filtering logic in readable form; not the main deliverable |

> The C++ binary requires administrator privileges. This is enforced by an embedded `app.manifest` with `requireAdministrator` — Windows refuses to launch it without an admin token. **No UAC elevation prompt is triggered.**

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

## How It Works — C++ WFP Engine

<a id="how-it-works"></a>

`main.cpp` is a headless Windows GUI process (`/SUBSYSTEM:WINDOWS`) that uses the **Windows Filtering Platform** directly to install network block rules at the kernel ALE callout boundary.

### Evasion-oriented design

`fwpuclnt.dll` is loaded at runtime — no WFP functions appear in the import table. All function names and the DLL name itself are Base64-encoded:

```cpp
hFwpDll = LoadLibraryA(Base64Decode("ZndwdWNsbnQuZGxs").c_str()); // fwpuclnt.dll
pFwpmFilterAdd0 = (FwpmFilterAdd0_t)GetProcAddress(hFwpDll, Base64Decode("RndwbUZpbHRlckFkZDA=").c_str());
```

AV endpoint domains and executable paths are also Base64-encoded — no plaintext strings in the binary.

### Sublayer & filter priority

A custom WFP sublayer is registered at weight `0xFFFF` (highest priority). Two filter types are installed:

**1. App identity filters** — `FWPM_LAYER_ALE_AUTH_CONNECT_V4/V6` + `FWPM_CONDITION_ALE_APP_ID`:
- Known AV executable directories → `FWP_ACTION_BLOCK` (weight 15)
- Known browser executables → `FWP_ACTION_PERMIT` (weight 14, overrides domain blocks for browsers)

**2. IP address filters** — `FWPM_CONDITION_IP_REMOTE_ADDRESS`:
A background thread resolves each blocked domain via `getaddrinfo` every 5 minutes, deduplicates the IPs, clears old filters, and installs fresh `/32` `FWP_ACTION_BLOCK` rules.

### No persistent state

The WFP session is opened with `FWPM_SESSION_FLAG_DYNAMIC` — all filters are automatically removed when the process exits. A hidden message-loop window (`CreateWindowEx`, not visible) keeps the process alive and provides a clean `WM_DESTROY` shutdown path.

### Admin requirement

`app.manifest` sets `requestedExecutionLevel` to `requireAdministrator`. The OS enforces this at launch — the process token either already carries admin rights or the launch is rejected outright.

---

## Reference Implementation (Python)

<a id="reference-impl"></a>

> **`example.py` is a high-level prototype**, written to make the filtering logic readable and easy to study. It is **not** the main component of this PoC. It uses third-party libraries (`proxy.py`, `psutil`) which the C++ implementation avoids entirely.

`example.py` models the same concept through an HTTP proxy:

- Starts `proxy.py` on `127.0.0.1:8899`
- Writes the proxy address to `HKCU`, `HKLM`, and WinHTTP so system services route through it
- `AdblockPlugin` per-request logic:
  - Resolves the requesting process: TCP port → PID → `.exe` path via `psutil`
  - AV processes → `403 Blocked app`
  - AV domains (wildcard `fnmatch`) → `403 Blocked domain`
  - Whitelisted browsers → pass-through

`reset_proxy.py` clears all registry and WinHTTP proxy settings if the process crashes and leaves the system offline.

---

## Why This Fails

<a id="why-this-fails"></a>

> **TL;DR: User-mode filtering cannot stop kernel-mode AV network stacks.**

| Limitation | Explanation |
|---|---|
| **HTTP proxy scope** | The Python layer only intercepts HTTP/HTTPS through the system proxy. AV clients use their own TLS stacks and raw sockets — proxy settings are ignored. |
| **WFP ALE layer** | `FWPM_LAYER_ALE_AUTH_CONNECT` fires at user-mode socket creation. AV kernel minifilters and NDIS drivers send traffic below this layer. |
| **Protected Process Light** | Windows Defender and others run as PPL. Their network I/O is not subject to WFP callout matching by unprivileged providers. |
| **AV owns WFP too** | Enterprise AVs register their own higher-priority WFP providers. They can delete foreign filters or install a `FWP_ACTION_PERMIT` that outweighs any block. |
| **CDN IP rotation** | The IP-block approach resolves A records at the time of execution. CDN-backed AV infrastructure serves different IPs constantly — any address resolved after the last refresh cycle is unrestricted. |
| **Dynamic session** | `FWPM_SESSION_FLAG_DYNAMIC` means every filter disappears the instant the process is killed. No persistence, no resilience. |

**This tool cannot meaningfully disrupt any modern AV product in a real environment.**

---

## Project Structure

<a id="project-structure"></a>

```
AVNetBlind/
├── main.cpp           # ★ Primary PoC — C++ WFP engine (headless, no console)
├── example.py         #   Reference prototype only — Python HTTP proxy equivalent
├── reset_proxy.py     #   Utility — resets all system proxy registry state
├── resource.rc        #   Win32 resources (version info, icon, manifest)
├── app.manifest       #   requireAdministrator — no UAC prompt, hard admin requirement
├── icon.ico           #   Application icon
└── build.bat          #   MSVC build (cl.exe + rc.exe, x64)
```

---

## Getting Started

<a id="getting-started"></a>

### Prerequisites

<a id="prerequisites"></a>

- Windows 10/11 (x64)
- Visual Studio 2022 — "Desktop development with C++" workload
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

The resulting `main.exe` has the manifest embedded. Launch it — if not already running as admin, Windows will reject the launch.

---

## Blocklist

<a id="blocklist"></a>

Update, telemetry, and cloud endpoints for **30+ vendors**:

`McAfee / Trellix` · `Trend Micro` · `Avira` · `Kaspersky` · `Bitdefender` · `Malwarebytes` · `ESET` · `Avast` · `AVG` · `Norton / Symantec` · `Windows Defender` · `SentinelOne` · `Cylance (Blackberry)` · `FireEye` · `Fortinet` · `Heimdal` · `Cybereason` · `Webroot` · `Comodo` · `DrWeb` · `Emsisoft` · `ZoneAlarm` · `VIPRE` · `AhnLab` · `Quick Heal` · `K7` · `360 Total Security` · `IObit` · `Panda` · `F-Secure`

All domain and path lists are Base64-encoded in source — **no plaintext strings** in the compiled binary.

---

## Detection & VT Score

<a id="detection"></a>

`main.exe` — **2 / 70** on VirusTotal.

Contributing factors:
- No WFP functions in import table (`LoadLibrary` + `GetProcAddress` at runtime)
- No plaintext AV domain or path strings
- `/SUBSYSTEM:WINDOWS` + hidden message loop — indistinguishable from a normal background GUI app
- Zero persistent state (`FWPM_SESSION_FLAG_DYNAMIC`)

---

## Disclaimer

<a id="disclaimer"></a>

> This project is published **for educational and research purposes only.**
>
> It is a proof-of-concept exploring a category of technique. It **cannot** meaningfully disrupt any real antivirus product. Do not deploy this on systems you do not own or have explicit written permission to test.
>
> The author assumes no liability for misuse.

---

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- MARKDOWN LINKS -->
[stars-shield]: https://img.shields.io/github/stars/your_username/AVNetBlind.svg?style=for-the-badge
[stars-url]: https://github.com/your_username/AVNetBlind/stargazers
[issues-shield]: https://img.shields.io/github/issues/your_username/AVNetBlind.svg?style=for-the-badge
[issues-url]: https://github.com/your_username/AVNetBlind/issues
[license-shield]: https://img.shields.io/badge/license-MIT-blue.svg?style=for-the-badge
[license-url]: https://github.com/your_username/AVNetBlind/blob/master/LICENSE