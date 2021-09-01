# Summary
making a game prototype

# Instructions
### Source and data
```
framework .... lower-level layer; OS interface, graphics interface; universal framework code
application .. middle-level layer; abstracts OS interaction, based on framework layer; provides universal game loop
prototype .... higher-level layer; game code, based on application layer; non-universal-yet code
third_party .. third-party code; some specifications
assets ....... binary data for the prototype layer
project ...... build scripts, debug scripts; target translation units; current changes, future plans
```

### IDE
```
__sublime_project ... project, intellisense, reference settings
__vscode_workspace .. project, intellisense, reference settings

you are free to use any other IDE, it's just me who didn't supply additional project files/options
```

### Compile and launch
> Manual:
1) from the project root, open `project`
2) run `build_clang.bat` or `build_msvc.bat`
3) from the project root, run `bin/game.exe`  
   N.B. code expects `assets` folder right at the current working directory

> IDE:
1) open ST4 or VSCode
2) launch one of the build/debug/run commands  
   N.B. debugging in Sublime Text relies on [RemedyBG](https://remedybg.itch.io/remedybg)

### Third-party code used
1) [stb](https://github.com/nothings/stb): single-file public domain (or MIT licensed) libraries for C/C++
2) [GL](https://github.com/KhronosGroup/OpenGL-Registry/tree/master/api/GL): OpenGL API and Extension Header Files  
   see also [Khronos OpenGL® Registry](https://www.khronos.org/registry/OpenGL/index_gl.php)
3) [KHR](https://github.com/KhronosGroup/EGL-Registry/tree/master/api/KHR): Khronos Shared Platform Header  
   see also [Khronos EGL Registry](https://www.khronos.org/registry/EGL/)
4) diverse RFC specifications (might be outdated)  
   [UTF-8](https://www.rfc-editor.org/rfc/rfc3629.txt), a transformation format of ISO 10646  
   [UTF-16](https://www.rfc-editor.org/rfc/rfc2781.txt), an encoding of ISO 10646  
   [PNG](https://www.rfc-editor.org/rfc/rfc2083.txt), (Portable Network Graphics) Specification Version 1.0  
   [DEFLATE](https://www.rfc-editor.org/rfc/rfc1951.txt), Compressed Data Format Specification version 1.3  
   [ZLIB](https://www.rfc-editor.org/rfc/rfc1950.txt), Compressed Data Format Specification version 3.3  
   [JSON](https://www.rfc-editor.org/rfc/rfc8259.txt), (The JavaScript Object Notation) Data Interchange Format  
