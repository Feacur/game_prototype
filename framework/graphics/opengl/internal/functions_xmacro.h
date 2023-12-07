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
//    XMACRO(DEPTHRANGE, DepthRange)
// >= 4.1
XMACRO(CLEARDEPTHF, ClearDepthf)
XMACRO(DEPTHRANGEF, DepthRangef)

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
XMACRO(ATTACHSHADER, AttachShader)
XMACRO(DETACHSHADER, DetachShader)
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
// >= 4.1
XMACRO(PROGRAMUNIFORM1FV, ProgramUniform1fv)
XMACRO(PROGRAMUNIFORM2FV, ProgramUniform2fv)
XMACRO(PROGRAMUNIFORM3FV, ProgramUniform3fv)
XMACRO(PROGRAMUNIFORM4FV, ProgramUniform4fv)
//
XMACRO(PROGRAMUNIFORM1IV, ProgramUniform1iv)
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
XMACRO(DELETETEXTURES, DeleteTextures)
// >= 4.5
XMACRO(CREATETEXTURES,     CreateTextures)
XMACRO(TEXTURESTORAGE2D,   TextureStorage2D)
XMACRO(TEXTUREPARAMETERI,  TextureParameteri)
XMACRO(TEXTUREPARAMETERIV, TextureParameteriv)
XMACRO(TEXTUREPARAMETERF,  TextureParameterf)
XMACRO(TEXTUREPARAMETERFV, TextureParameterfv)
XMACRO(TEXTURESUBIMAGE2D,  TextureSubImage2D)
XMACRO(BINDTEXTUREUNIT,    BindTextureUnit)
XMACRO(GENERATETEXTUREMIPMAP, GenerateTextureMipmap)

// -- target
// >= 3.0
XMACRO(DELETEFRAMEBUFFERS,  DeleteFramebuffers)
XMACRO(DELETERENDERBUFFERS, DeleteRenderbuffers)
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
XMACRO(GENBUFFERS,    GenBuffers)
XMACRO(DELETEBUFFERS, DeleteBuffers)
// >= 3.0
XMACRO(DELETEVERTEXARRAYS, DeleteVertexArrays)
XMACRO(BINDVERTEXARRAY,    BindVertexArray)
// >= 4.5
XMACRO(CREATEVERTEXARRAYS,       CreateVertexArrays)
XMACRO(CREATEBUFFERS,            CreateBuffers)
XMACRO(VERTEXARRAYELEMENTBUFFER, VertexArrayElementBuffer)
XMACRO(VERTEXARRAYVERTEXBUFFER,  VertexArrayVertexBuffer)
XMACRO(ENABLEVERTEXARRAYATTRIB,  EnableVertexArrayAttrib)
XMACRO(DISABLEVERTEXARRAYATTRIB, DisableVertexArrayAttrib)
XMACRO(VERTEXARRAYATTRIBBINDING, VertexArrayAttribBinding)
XMACRO(VERTEXARRAYATTRIBFORMAT,  VertexArrayAttribFormat)
XMACRO(NAMEDBUFFERSTORAGE,       NamedBufferStorage)
XMACRO(NAMEDBUFFERSUBDATA,       NamedBufferSubData)

// -- storage
// >= 3.0
XMACRO(BINDBUFFERRANGE, BindBufferRange)

// -- display
// >= 2.0
XMACRO(VIEWPORT,        Viewport)
XMACRO(BINDFRAMEBUFFER, BindFramebuffer)
//
XMACRO(CLEAR,        Clear)
XMACRO(DRAWELEMENTSINSTANCED, DrawElementsInstanced)
XMACRO(DRAWARRAYSINSTANCED,   DrawArraysInstanced)
XMACRO(FINISH,       Finish)
XMACRO(FLUSH,        Flush)

#undef XMACRO
