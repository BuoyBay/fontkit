---
title: "fontkit typed API"
summary: "Source-derived public signatures, properties, parameters, defaults, and return values."
category: "Reference"
order: 21
types: ["FontFaceInfo", "FontKitException", "FontRegistration"]
dependencies: []
symbols: ["FontFaceInfo", "FontKitException", "FontRegistration"]
related: ["api.md", "getting-started.md", "index.md"]
---

# fontkit typed API

Generated from the package source declarations. Platform guards are shown where present; inherited members are documented on the base type. See the [API contracts](api.md) for behavior and limitations.

## FontKitException

`Public Class FontKitException Inherits Exception`

### Constructor

`Public Sub Constructor(message As String)`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `message` | `String` | `ByVal` | Required |

Returns: no value.

## FontFaceInfo

`Public Class FontFaceInfo`

Native metadata; empty fields mean the backend did not expose that value.

### Properties

| Name | Type | Read-Only | Shared | Default | Description |
| --- | --- | --- | --- | --- | --- |
| `FamilyName` | `String` | No | No | — | — |
| `FaceName` | `String` | No | No | — | — |
| `PostScriptName` | `String` | No | No | — | — |

## FontKit functions

### FontKitBackend

`Public Function FontKitBackend() As String`

No parameters.

#### Returns

| Type |
| --- |
| `String` |

### FontKitHasPostScriptMetadata

`Public Function FontKitHasPostScriptMetadata() As Boolean`

No parameters.

#### Returns

| Type |
| --- |
| `Boolean` |

### FontKitCanUnload

`Public Function FontKitCanUnload() As Boolean`

No parameters.

#### Returns

| Type |
| --- |
| `Boolean` |

### FontKitGuaranteesCanvasVisibility

`Public Function FontKitGuaranteesCanvasVisibility() As Boolean`

No rendering-backend visibility guarantee follows from registration.

No parameters.

#### Returns

| Type |
| --- |
| `Boolean` |

### FontKitFaces

`Public Function FontKitFaces() As Array Of FontFaceInfo`

Snapshot of native faces; order, localization, and duplicates are native-defined.

No parameters.

#### Returns

| Type |
| --- |
| `Array Of FontFaceInfo` |

### FontKitHasFamily

`Public Function FontKitHasFamily(familyName As String) As Boolean`

Exact case-sensitive native family-name presence, without fallback or aliases.

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `familyName` | `String` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `Boolean` |

## FontRegistration

`Public Class FontRegistration`

Keep alive while using the font. Serialize all FontKit calls on one thread.

### Properties

| Name | Type | Read-Only | Shared | Default | Description |
| --- | --- | --- | --- | --- | --- |
| `IsOpen` | `Boolean` | Yes | No | — | — |

### Constructor

`Public Sub Constructor(filePath As String)`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `filePath` | `String` | `ByVal` | Required |

Returns: no value.

### Close

`Public Sub Close()`

Idempotent; native unregistration failures throw and leave the handle open.

No parameters.

Returns: no value.

### Destructor

`Public Sub Destructor()`

No parameters.

Returns: no value.
