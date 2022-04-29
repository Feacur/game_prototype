#if !defined (XMACRO)
	#define XMACRO(type, name)
#endif

// -- init
// >= 2.0
XMACRO(GETINTEGERV, GetIntegerv)
XMACRO(GETSTRING,   GetString)

#if defined(XMACRO_INIT)
	XMACRO_INIT()
	#undef XMACRO_INIT
#endif

// -- extensions
// >= 3.0
XMACRO(GETSTRINGI, GetStringi)

// -- settings
// >= 2.0
XMACRO(DISABLE, Disable)
XMACRO(ENABLE,  Enable)
//
//    XMACRO(DEPTHRANGE, DepthRange)
// >= 4.1
XMACRO(DEPTHRANGEF, DepthRangef)
// >= 4.3
XMACRO(DEBUGMESSAGECALLBACK, DebugMessageCallback)
XMACRO(DEBUGMESSAGECONTROL,  DebugMessageControl)
// >= 4.5
XMACRO(CLIPCONTROL, ClipControl)

// -- color
// >= 2.0
XMACRO(CLEARCOLOR,            ClearColor)
XMACRO(COLORMASK,             ColorMask)
XMACRO(BLENDCOLOR,            BlendColor)
XMACRO(BLENDFUNCSEPARATE,     BlendFuncSeparate)
XMACRO(BLENDEQUATIONSEPARATE, BlendEquationSeparate)
//    XMACRO(BLENDFUNC,     BlendFunc)
//    XMACRO(BLENDEQUATION, BlendEquation)
//    XMACRO(DRAWBUFFERS,   DrawBuffers)
// >= 3.0
//    XMACRO(CLEARBUFFERIV,  ClearBufferiv)
//    XMACRO(CLEARBUFFERUIV, ClearBufferuiv)
//    XMACRO(CLEARBUFFERFI,  ClearBufferfi)
//    XMACRO(CLEARBUFFERFV,  ClearBufferfv)
// >= 4.0
//    XMACRO(BLENDEQUATIONSEPARATEI, BlendEquationSeparatei)
//    XMACRO(BLENDEQUATIONSEPARATEI, BlendEquationSeparatei)
// >= 4.5
XMACRO(NAMEDFRAMEBUFFERDRAWBUFFERS, NamedFramebufferDrawBuffers)
XMACRO(CLEARNAMEDFRAMEBUFFERIV,     ClearNamedFramebufferiv)
XMACRO(CLEARNAMEDFRAMEBUFFERUIV,    ClearNamedFramebufferuiv)
XMACRO(CLEARNAMEDFRAMEBUFFERFV,     ClearNamedFramebufferfv)
XMACRO(CLEARNAMEDFRAMEBUFFERFI,     ClearNamedFramebufferfi)

// -- depth
// >= 2.0
XMACRO(DEPTHFUNC,  DepthFunc)
XMACRO(DEPTHMASK,  DepthMask)
//    XMACRO(CLEARDEPTH, ClearDepth)
// >= 4.1
XMACRO(CLEARDEPTHF, ClearDepthf)

// -- stencil
// >= 2.0
XMACRO(CLEARSTENCIL, ClearStencil)
//    XMACRO(STENCILMASK,  StencilMask)
//    XMACRO(STENCILFUNC,  StencilFunc)
//    XMACRO(STENCILOP,    StencilOp)

// -- culling
// >= 2.0
XMACRO(CULLFACE,  CullFace)
XMACRO(FRONTFACE, FrontFace)

// -- GPU program
// >= 2.0
XMACRO(ATTACHSHADER, AttachShader) // requires both 'program' and 'shader' ids
XMACRO(DETACHSHADER, DetachShader) // requires both 'program' and 'shader' ids
//
XMACRO(CREATEPROGRAM,     CreateProgram)
XMACRO(DELETEPROGRAM,     DeleteProgram)
XMACRO(USEPROGRAM,        UseProgram)
XMACRO(LINKPROGRAM,       LinkProgram)
XMACRO(GETPROGRAMIV,      GetProgramiv)
XMACRO(GETPROGRAMINFOLOG, GetProgramInfoLog)
//    XMACRO(GETPROGRAMBINARY, GetProgramBinary)
//
XMACRO(CREATESHADER,     CreateShader)
XMACRO(DELETESHADER,     DeleteShader)
XMACRO(SHADERSOURCE,     ShaderSource)
XMACRO(COMPILESHADER,    CompileShader)
XMACRO(GETSHADERIV,      GetShaderiv)
XMACRO(GETSHADERINFOLOG, GetShaderInfoLog)
//    XMACRO(GETSHADERSOURCE, GetShaderSource)
// >= 4.3
XMACRO(GETPROGRAMINTERFACEIV,  GetProgramInterfaceiv)
XMACRO(GETPROGRAMRESOURCEIV,   GetProgramResourceiv)
XMACRO(GETPROGRAMRESOURCENAME, GetProgramResourceName)

// -- attributes (GPU program vertex locations)
// >= 2.0
//    XMACRO(GETACTIVEATTRIB,   GetActiveAttrib)
//    XMACRO(GETATTRIBLOCATION, GetAttribLocation)

// -- uniforms (GPU program variables and constants)
// >= 2.0
//    XMACRO(GETACTIVEUNIFORM,   GetActiveUniform)
//    XMACRO(GETUNIFORMLOCATION, GetUniformLocation)
// >= 3.1
//    XMACRO(GETACTIVEUNIFORMSIV,  GetActiveUniformsiv)
//    XMACRO(GETACTIVEUNIFORMNAME, GetActiveUniformName)
//
//    XMACRO(UNIFORM1FV, Uniform1fv)
//    XMACRO(UNIFORM2FV, Uniform2fv)
//    XMACRO(UNIFORM3FV, Uniform3fv)
//    XMACRO(UNIFORM4FV, Uniform4fv)
//
//    XMACRO(UNIFORM1IV, Uniform1iv) // also a sampler uniform
//    XMACRO(UNIFORM2IV, Uniform2iv)
//    XMACRO(UNIFORM3IV, Uniform3iv)
//    XMACRO(UNIFORM4IV, Uniform4iv)
//
//    XMACRO(UNIFORMMATRIX2FV, UniformMatrix2fv)
//    XMACRO(UNIFORMMATRIX3FV, UniformMatrix3fv)
//    XMACRO(UNIFORMMATRIX4FV, UniformMatrix4fv)
// >= 3.0
//    XMACRO(UNIFORM1UIV, Uniform1uiv)
//    XMACRO(UNIFORM2UIV, Uniform2uiv)
//    XMACRO(UNIFORM3UIV, Uniform3uiv)
//    XMACRO(UNIFORM4UIV, Uniform4uiv)
// >= 4.1
XMACRO(PROGRAMUNIFORM1FV, ProgramUniform1fv)
XMACRO(PROGRAMUNIFORM2FV, ProgramUniform2fv)
XMACRO(PROGRAMUNIFORM3FV, ProgramUniform3fv)
XMACRO(PROGRAMUNIFORM4FV, ProgramUniform4fv)
//
XMACRO(PROGRAMUNIFORM1IV, ProgramUniform1iv) // also a sampler uniform
XMACRO(PROGRAMUNIFORM2IV, ProgramUniform2iv)
XMACRO(PROGRAMUNIFORM3IV, ProgramUniform3iv)
XMACRO(PROGRAMUNIFORM4IV, ProgramUniform4iv)
//
XMACRO(PROGRAMUNIFORM1UIV, ProgramUniform1uiv)
XMACRO(PROGRAMUNIFORM2UIV, ProgramUniform2uiv)
XMACRO(PROGRAMUNIFORM3UIV, ProgramUniform3uiv)
XMACRO(PROGRAMUNIFORM4UIV, ProgramUniform4uiv)
//
XMACRO(PROGRAMUNIFORMMATRIX2FV, ProgramUniformMatrix2fv)
XMACRO(PROGRAMUNIFORMMATRIX3FV, ProgramUniformMatrix3fv)
XMACRO(PROGRAMUNIFORMMATRIX4FV, ProgramUniformMatrix4fv)

// -- textures
// >= 2.0
XMACRO(BINDTEXTURE,    BindTexture)
XMACRO(TEXIMAGE2D,     TexImage2D)
XMACRO(DELETETEXTURES, DeleteTextures)
//    XMACRO(GENTEXTURES,   GenTextures)
//    XMACRO(TEXPARAMETERI, TexParameteri)
//    XMACRO(TEXSUBIMAGE2D, TexSubImage2D)
// >= 4.2
//    XMACRO(TEXSTORAGE2D, TexStorage2D)
// >= 4.5
XMACRO(CREATETEXTURES,     CreateTextures)
XMACRO(TEXTURESTORAGE2D,   TextureStorage2D)
XMACRO(TEXTUREPARAMETERI,  TextureParameteri)
XMACRO(TEXTUREPARAMETERIV, TextureParameteriv)
XMACRO(TEXTURESUBIMAGE2D,  TextureSubImage2D)
XMACRO(BINDTEXTUREUNIT,    BindTextureUnit)

// -- target
// >= 3.0
XMACRO(DELETEFRAMEBUFFERS,  DeleteFramebuffers)
XMACRO(DELETERENDERBUFFERS, DeleteRenderbuffers)
//    XMACRO(GENFRAMEBUFFERS,      GenFramebuffers)
//    XMACRO(FRAMEBUFFERTEXTURE2D, FramebufferTexture2D)
// >= 4.5
XMACRO(CREATEFRAMEBUFFERS,           CreateFramebuffers)
XMACRO(NAMEDFRAMEBUFFERTEXTURE,      NamedFramebufferTexture)
XMACRO(CREATERENDERBUFFERS,          CreateRenderbuffers)
XMACRO(NAMEDRENDERBUFFERSTORAGE,     NamedRenderbufferStorage)
XMACRO(NAMEDFRAMEBUFFERRENDERBUFFER, NamedFramebufferRenderbuffer)

// -- samplers
// >= 3.2
//    XMACRO(GENSAMPLERS,       GenSamplers)
//    XMACRO(DELETESAMPLERS,    DeleteSamplers)
//    XMACRO(BINDSAMPLER,       BindSampler)
//    XMACRO(SAMPLERPARAMETERI, SamplerParameteri)
// >= 4.5
//    XMACRO(CREATESAMPLERS, CreateSamplers)

// -- meshes
// >= 2.0
XMACRO(GENBUFFERS,               GenBuffers)
XMACRO(DELETEBUFFERS,            DeleteBuffers)
//    XMACRO(BINDBUFFER,               BindBuffer)
//    XMACRO(BUFFERDATA,               BufferData)
//    XMACRO(DISABLEVERTEXATTRIBARRAY, DisableVertexAttribArray)
//    XMACRO(ENABLEVERTEXATTRIBARRAY,  EnableVertexAttribArray)
//    XMACRO(VERTEXATTRIBPOINTER,      VertexAttribPointer)
//    XMACRO(BUFFERSUBDATA,            BufferSubData)
// >= 3.0
XMACRO(DELETEVERTEXARRAYS, DeleteVertexArrays)
XMACRO(BINDVERTEXARRAY,    BindVertexArray)
//    XMACRO(GENVERTEXARRAYS,    GenVertexArrays)
// >= 4.3
//    XMACRO(BINDVERTEXBUFFER,    BindVertexBuffer)
//    XMACRO(VERTEXATTRIBFORMAT,  VertexAttribFormat)
//    XMACRO(VERTEXATTRIBBINDING, VertexAttribBinding)
// >= 4.5
XMACRO(CREATEVERTEXARRAYS,       CreateVertexArrays)
XMACRO(CREATEBUFFERS,            CreateBuffers)
XMACRO(NAMEDBUFFERDATA,          NamedBufferData)
XMACRO(VERTEXARRAYELEMENTBUFFER, VertexArrayElementBuffer)
XMACRO(VERTEXARRAYVERTEXBUFFER,  VertexArrayVertexBuffer)
XMACRO(ENABLEVERTEXARRAYATTRIB,  EnableVertexArrayAttrib)
XMACRO(VERTEXARRAYATTRIBFORMAT,  VertexArrayAttribFormat)
XMACRO(VERTEXARRAYATTRIBBINDING, VertexArrayAttribBinding)
XMACRO(NAMEDBUFFERSTORAGE,       NamedBufferStorage)
XMACRO(NAMEDBUFFERSUBDATA,       NamedBufferSubData)
//    XMACRO(DISABLEVERTEXARRAYATTRIB, DisableVertexArrayAttrib)

// -- display
// >= 2.0
XMACRO(VIEWPORT,        Viewport)
XMACRO(BINDFRAMEBUFFER, BindFramebuffer)
//
XMACRO(CLEAR,        Clear)
XMACRO(DRAWELEMENTS, DrawElements)
XMACRO(DRAWARRAYS,   DrawArrays)
XMACRO(FINISH,       Finish)
XMACRO(FLUSH,        Flush)

#undef XMACRO
