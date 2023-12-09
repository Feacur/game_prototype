# To do
- [feature] immediate UI framework
- [tech] imUI transformations stack
- [tech] render text via - multichannel - SDF glyphs
- [tech] optionally generate ligature codepoints at the time of decoding UTF-8
- [tech] track materials' textures as assets
- [feature] default shaders in case of compilation errors
- [tech] compile thirdparties as separate units
- [tech] builder with file time or hash tracking
- [tech] allow removing and updating glyph atlases' ranges at runtime via code
- [bug] glyph atlas metrics should account for multiple typefaces
- [feature] allow scaling glyph atlas range
- [feature] introduce persistent ranges for glyph atlases, which can be rendered beforehand and kept indefinitely
- [tech] expose GPU samplers
- [tech] handle GPU samplers' lifetime
- [feature] allow assets to be dynamic
- [tech] update assets dependecies, like if swapping materials' shaders or textures
- [tech] consider dropping resources on the floor on exit

# Changelog

## 2023.12.09
- [tech] handle GPU uniforms through buffers
- [tech] region-based memory allocator, kinda

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
