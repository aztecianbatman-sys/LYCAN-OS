# LYCAN 1.0 architecture

LYCAN has four boundaries: the Windows host, the LYCAN runtime, the guest filesystem, and guest applications.

The ARES VM is deliberately small and deterministic: registers, program counter, RAM, protected page zero, and a finite instruction budget. The VM is a real interpreter, not a screenshot or mock status screen.

LYFS maps guest paths into `%LOCALAPPDATA%\\LycanOS\\data\\lyfs` and rejects traversal outside that root. Process records are guest-owned. The package manager refuses untrusted publishers and stores installed manifests under `/apps`.

The desktop is a presentation layer over the same runtime. Terminal commands operate against guest state; Files reads LYFS; Diagnostics reports VM and policy state; Store exposes package metadata; Web is reserved for a genuine Gecko integration.

The Windows installer installs one ordinary executable and supporting data. It does not modify the Windows bootloader, replace the Windows kernel, or create a dual-boot installation.
