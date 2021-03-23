# framework
```
[ ] text rendering
    [ ] GPU texture data streaming, probably, too?
    [ ] improve font glyphs packer (probably make it a sprite/rect packer)
[ ] graphics
    [ ] API to correctly batch-update meshes/textures and create a render pass (or whatever)
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
[ ] organize standard includes?
[ ] should batch_mesh and font_image belong to some other folder?
[ ] support large files? more than 2gb? more than 4gb?
[ ] async file access?
[ ] reuse mesh vertices at `asset_mesh_obj_repack` function
[ ] UTF-8 edge cases, different languages, LTR/RTL, etc.
[ ] clean up containers implementation
    [ ] ever need a uint64_t hashes for hash tables?
[ ] build system
    [ ] streamline build settings switching
```

[Markdown](https://www.markdownguide.org/basic-syntax/)
