# FontKit 0.1.0

Font discovery and process-local font-file registration for Buoy 0.72.0.
This package owns discovery/loading only; it does not choose fallback fonts,
measure text, shape text, or install fonts for other applications.

## API

Import `fontkit`. See `tests/fontkit_test.bui` for a compiler-checked usage example.

| API | Behavior |
| --- | --- |
| `FontKitFaces()` | An array of `FontFaceInfo` objects copied from a native snapshot. |
| `FontFaceInfo.FamilyName` | Native family name, suitable for exact discovery checks. |
| `FontFaceInfo.FaceName` | Native full face name, not a synthesized style or stable identifier. |
| `FontFaceInfo.PostScriptName` | Native PostScript name where exposed; empty when unavailable. |
| `FontKitHasFamily(name)` | Exact, case-sensitive family presence in a fresh snapshot. No aliases or fallback matching. |
| `FontKitBackend()` | Identifies the registration/discovery backend. |
| `FontKitHasPostScriptMetadata()` | Whether the backend can expose PostScript names; individual faces can still have empty metadata. |
| `FontKitCanUnload()` | Whether closing the last registration handle attempts native unregistration. |
| `FontKitGuaranteesCanvasVisibility()` | Always false: registration does not prove visibility in every Buoy drawing/UI backend. |
| `New FontRegistration(filePath)` | Registers an existing font file or throws. Exposes `IsOpen` and idempotent `Close()`. |

Metadata reflects native enumeration, including its ordering, localization,
possible duplicates, and omissions. No cross-platform sorting or face identity is
implied. Windows GDI's empty-family enumeration can expose representative faces
rather than every style. Enumeration errors throw `FontKitException`, rather than
returning a misleading empty success. Empty paths and NUL bytes still visible
to the wrapper throw `InvalidArgumentException`; missing/invalid/native-rejected files throw
`FontKitException`. Native registration errors are intentionally generic where
the native API does not provide details. Paths are UTF-8; Windows converts to
UTF-16. Font collections are accepted only if the backend accepts the file.

Embedded NUL cannot be reliably validated through this Buoy String ABI: the
tested runtime discards `String.Chr(0)` before the wrapper can inspect it. The
byte check rejects only NUL that survives String construction. No end-to-end
NUL-rejection guarantee is claimed; callers handling binary paths need upstream
length-bearing String/FFI support. The suffixed-path rejection test does not prove
embedded-NUL detection.

## Ownership and visibility

Serialize all FontKit calls on one application thread. This includes destructors:
create, use and release handles on that thread. Keep each registration alive
while font-dependent objects exist. Close rendering objects before closing the
last font registration on platforms that unload. The package cannot track fonts
retained inside Buoy's runtime or third-party rendering objects.

Each object owns one lease. Repeated canonical path registrations share a native
registration; closing one lease preserves the others. Unix paths use `realpath`
(including symlink resolution). Windows uses an absolute full path and exact
spelling; different case/hardlink spellings may acquire separate native resources.
Do not replace or remove a registered file while using it.

`Close()` explicitly reports unregistration failure and leaves the handle open
for retry. The destructor attempts cleanup without throwing; native cleanup
failures at that point can retain registration state until process exit. Use
explicit `Close()` when failures must be observed. Registrations made outside
FontKit are never adopted or unregistered by FontKit; a native duplicate conflict
can therefore fail construction.

| Platform | Discovery | Registration and lifetime | Rendering scope |
| --- | --- | --- | --- |
| macOS | Core Text available PostScript names, resolved family/full names | Core Text process scope; last lease unregisters | Core Text matching; other caches/backends are not guaranteed |
| Windows | GDI `EnumFontFamiliesExW`; family/full names, no PostScript name | GDI `AddFontResourceExW(FR_PRIVATE)`; last lease calls matching removal | GDI process-private fonts; **does not populate a GDI+ `PrivateFontCollection`** |
| Linux | Fontconfig current configuration; family/full/PostScript fields | `FcConfigAppFontAddFile` on current config; font remains until process exit | Invalidates the existing default PangoCairo Fc font map only |

Linux deliberately reports `FontKitCanUnload() = False`. Fontconfig's public
removal API clears **all** application fonts in a configuration. FontKit never
calls it, so it does not erase registrations owned by other components. Closing
a Linux handle releases that lease, while a small registry entry and the native
font remain until process exit. Reopening the same canonical path reuses that
entry. Changing the process's current Fontconfig configuration externally after
registration is outside this contract. Do not use a permanently changing stream
of font paths with this process-lifetime API.

Linux invalidation covers the default PangoCairo font map when it is a PangoFc
map. Custom Pango maps, maps with a separate configuration, existing text layouts,
and other renderer caches require their owners to refresh them. Canvas and native
controls are not claimed to switch fonts merely because discovery sees a family.
On Windows, supporting Buoy Canvas through its own GDI+ private collection requires
runtime integration; there is no supported collection-injection API here. These
are visibility constraints, not successful-rendering assertions.
For explicit Windows private-font Picture rendering, use the separate FontCanvas
package; it owns its GDI+ collection and requires no runtime injection.

## Build and verify

Run from the repository root:

```
python3 tools/build_fontkit.py
```

The builder compiles the host native bridge, checks the Buoy module for all six
desktop targets, then seals `packages/fontkit-0.1.0.buipkg` and copies it to `dist/`.
It stages only native binaries whose receipt matches the current bridge source,
and verifies their binary architecture. It does not fabricate missing slices.
Use `--native-only`, `--seal-only`, `--target`, and `--cc` for native/cross toolchain
builds. `BUOY`, `BUOY_PACKAGER`, and `CC` override tools; Linux cross builds also
require target-specific pkg-config paths/libraries. Linux needs Fontconfig,
PangoCairo and PangoFT2 development packages. macOS needs the Apple SDK;
Windows needs GDI32 and a matching C compiler/import-library toolchain.

The produced archive contains all **six desktop architecture slices**: x86-64
and ARM64 Linux, Windows and macOS. Linux x86-64, Windows x86-64 and macOS ARM64
consumers have executed on their actual operating systems. Linux ARM64 native
libraries and the full Buoy consumer passed under userspace QEMU on x86-64;
this is emulated execution, not native ARM hardware. Windows ARM64 DLL/import
library and a sealed Buoy consumer cross-linked successfully but were not run.
Both macOS slices were SDK-built; Intel macOS execution remains unverified
because Rosetta is unavailable. See `tests/font_arm_evidence/README.md` for
reproducible isolated ARM build/execution steps and exact scope.
All six Buoy compiler checks validate syntax/types only; native evidence is
separate and does not establish rendering-backend visibility.

Maintainers run the private validation suite before release. See the repository README for public build instructions.

The verifier checks the module and test source on six targets, builds a consumer
against the sealed archive, and runs host tests for enumeration, copied metadata,
unknown family rejection, invalid/missing paths, fixture registration,
duplicate canonical path leases, repeated Close, destructor ownership and the
reported unload behavior. Linux additionally primes a default Pango font map
before registration and checks that the existing map discovers the new family.
The fixture is an original 1 KB CC0 TTF with a unique family name; its hash is
pinned in the verifier. `fixtures/generate.py` regenerates it with
`fonttools==4.60.1`, which is not needed to build or run the package.

No upstream compiler/runtime sources were modified. The package cannot fix
runtime-owned GDI+ collections or guarantee cache refresh in every drawing backend.

## Native references

- [Core Text process registration](https://developer.apple.com/documentation/coretext/ctfontmanagerregisterfontsforurl(_:_:_:))
- [Windows private GDI fonts](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-addfontresourceexw)
- [GDI+ private collection ownership](https://learn.microsoft.com/en-us/windows/win32/gdiplus/-gdiplus-creating-a-private-font-collection-use)
- [Fontconfig application fonts and clear-all removal](https://fontconfig.pages.freedesktop.org/fontconfig/fontconfig-devel/)
- [PangoFc configuration invalidation](https://docs.gtk.org/PangoFc/method.FontMap.config_changed.html)
