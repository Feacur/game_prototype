# framework
```
[ ] entity system
    [ ] operate entities and components
[ ] asset system
[ ] text rendering
    [x] automatically build atlas based on input text
    [ ] select suitable fonts for different locales
    [x] grabage collect glyphs
    [x] collect all the relevant glyphs, then update textures AND texture coordinates
    [ ] decode UTF-8 once into an array?
    [x] glyphs GC threshold
    [ ] tune/expose glyphs GC
    [x] postpone fonts atlases generation and texts quads creation
[ ] batch rendering
    [ ] direct static parts into separate batchers
        [ ] cache vertices and stuff until changed
        [ ] warn/fail upon a change
    [ ] add a 3d batcher?
[ ] graphics
    [ ] remove mutable buffers, but recreate immutable ones completely?
    [ ] elaborate the idea of render passes
    [ ] GPU scissor test
    [ ] support older OpenGL versions (pre direct state access, which is 4.5)
    [ ] storage changes?
        [ ] condense graphics material's buffers into a single bytes arrays?
        [ ] condense asset mesh's buffers into a single bytes arrays?
        [ ] use flexible array members with opaque pointers API?
    [ ] expose screen buffer settings, as well as OpenGL's
[ ] platform
    [ ] elaborate raw input
    [ ] expose Caps Lock and Num Lock toggle states?
[ ] memory
    [ ] universal scratch memory allocator
    [ ] allocators: stack (bulk-free only), pool (fixed chunks allocation only), free (completely custom)
    [ ] use OS-native allocators
    [ ] mind the alignment
[ ] organize standard includes?
[ ] async file access? (through OS API)
[ ] reuse mesh vertices at `mesh_obj_repack` function
[ ] UTF-8 edge cases, different languages, LTR/RTL, ligatures, etc.
[ ] make a universal sprite packer?
[ ] try custom sorting algorithms?
[ ] clean up containers implementation
    [ ] ever need a uint64_t hashes for hash tables?
    [x] `set` methods for arrays
    [ ] bound checks? (means, more of them)
[ ] build system
    [ ] streamline build settings switching
```

[Markdown](https://www.markdownguide.org/basic-syntax/)
