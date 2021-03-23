# 2021, march 24
```
[o] text rendering
    [x] UTF-8 decoding
[o] build system
	[x] make translation units list explicit
```

# 2021, march 21
```
 - framework
[o] text rendering
    [x] GPU mesh data streaming
    [x] 2d batch renderer (quite universal, actually, but needs testing)
    [x] asset loader: *.ttf
    [x] simplistic font glyphs packer
    [x] hashtable for glyphs (turned out to be quite generic)
```

# 2021, march 19
```
--- a mind dump of the current state ---
 - framework
[x] centralized memory allocator
[x] abstract platform layer
    [x] graphics library initialization: WGL -> OpenGL
    [x] filesystem access
    [x] mouse/keyboard input
[x] abstract graphics layer
    [x] GPU objects: program, texture, target, mesh
    [x] a naive render pass block
[x] asset loaders: *.png, *.obj
[x] a set of container objects
[x] maths
 - application
[x] automatically initialize the platform layer
[x] expose callbacks API to the prototype
 - project
[x] custom MSVC/Clang build "system" (direct use of compilators and linkers)
[x] unity build option
```

[Markdown](https://www.markdownguide.org/basic-syntax/)
