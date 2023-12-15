# To do
- [prototype] ugh, focus on some gameplay, it always helps with underlying tech and features
- [feature] immediate UI framework
- [tech] track materials' textures as assets
- [tech] hot reloading of assets
- [tech] hot reloading of DLL modules; treat them as assets? needs a set of API function-pointer
- [tech] expose GPU samplers instead of embedding them within textures
- [bug] glyph atlas metrics should account for multiple typefaces; not sure how
- [feature] allow scaling glyph atlas range on top of font size
- [feature] introduce persistent ranges for glyph atlases, which can be rendered beforehand and kept indefinitely
- [tech] update assets dependecies, like if swapping materials' shaders or textures
- [tech] render text via - multichannel - SDF glyphs
- [tech] optionally generate ligature codepoints at the time of decoding UTF-8
- [feature] default shaders in case of compilation errors
- [tech] compile thirdparties as separate units
- [tech] builder with file time or hash tracking (like tsoding's `nobuild`)

# Changelog

## 2023.12.15
- [tech] a number of allocators, interchangeable to an extent: platform, generic, debug, arena
- [tech] improve batching by sampling multiple textures instead of constant context switches
- [tech] drop resources on the floor on exit, in release mode
- [tech] properly expose scan codes (physical) alongside key codes (logical)

## 2023.12.11
- [tech] make systems autonomous and clearable instead of initializable and freeable
- [tech] improve hashing

## 2023.12.09
- [tech] handle GPU uniforms via UBO, i.e. buffers
- [tech] region-based memory allocator, kinda
- [tech] dedicated defered actions system via `struct Handle`
- [tech] store OpenGL functions in a struct

## 2023.12.07
- [tech] instanced drawing via SSBO, i.e. buffers
- [tech] improve manifest: support UTF-8, remove path-limit
- [tech] improve resources: include manifest

## 2023.12.05
- [tech] optionally build via `zig cc`
- [tech] optionally build via translation units list

## 2023.10.21
- [tech] handle assets lifetime
- [tech] track assets dependencies
- [tech] log with tags
- [tech] materials pool (but... it was done already?)
- [tech] deduplicate build system
- [tech] add an icon, build and link resources
- [tech] handle DPI awareness via manifest (less code)

## 2023.02.28
- [tech] untie uniform names registry from the API implementation
- [tech] allow addressing uniforms by name

## 2023.02.26
- [tech] draw ImUI without Gfx_Material
- [bug] correctly batch shader ImUI changes

## 2023.02.02
- [feature] allow adding multiple typefaces per glyph atlas

[Markdown](https://www.markdownguide.org/basic-syntax/)
