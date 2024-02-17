#if !defined (XMACRO)
	#define XMACRO(type, name)
#endif

// ----- ----- ----- ----- -----
//     GPU info part
// ----- ----- ----- ----- -----

// >= 2.0
XMACRO(GETINTEGERV, GetIntegerv)
XMACRO(GETSTRING,   GetString)
// >= 3.0
XMACRO(GETSTRINGI, GetStringi)

// ----- ----- ----- ----- -----
//     GPU settings part
// ----- ----- ----- ----- -----

// >= 2.0
XMACRO(DISABLE, Disable)
XMACRO(ENABLE,  Enable)
// >= 4.3
XMACRO(DEBUGMESSAGECALLBACK, DebugMessageCallback)
XMACRO(DEBUGMESSAGECONTROL,  DebugMessageControl)
// >= 4.5
XMACRO(CLIPCONTROL, ClipControl)

// ----- ----- ----- ----- -----
//     GPU blending part
// ----- ----- ----- ----- -----

// >= 2.0
XMACRO(CLEARCOLOR,            ClearColor)
XMACRO(COLORMASK,             ColorMask)
XMACRO(BLENDCOLOR,            BlendColor)
XMACRO(BLENDFUNCSEPARATE,     BlendFuncSeparate)
XMACRO(BLENDEQUATIONSEPARATE, BlendEquationSeparate)

// ----- ----- ----- ----- -----
//     GPU depth part
// ----- ----- ----- ----- -----

// >= 2.0
XMACRO(DEPTHFUNC,  DepthFunc)
XMACRO(DEPTHMASK,  DepthMask)
//    XMACRO(CLEARDEPTH, ClearDepth)
//    XMACRO(DEPTHRANGE, DepthRange)
// >= 4.1
XMACRO(CLEARDEPTHF, ClearDepthf)
XMACRO(DEPTHRANGEF, DepthRangef)

// ----- ----- ----- ----- -----
//     GPU stencil part
// ----- ----- ----- ----- -----

// >= 2.0
XMACRO(CLEARSTENCIL, ClearStencil)
//    XMACRO(STENCILMASK,  StencilMask)
//    XMACRO(STENCILFUNC,  StencilFunc)
//    XMACRO(STENCILOP,    StencilOp)

// ----- ----- ----- ----- -----
//     GPU culling part
// ----- ----- ----- ----- -----

// >= 2.0
XMACRO(CULLFACE,  CullFace)
XMACRO(FRONTFACE, FrontFace)

// ----- ----- ----- ----- -----
//     GPU program part
// ----- ----- ----- ----- -----

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

// ----- ----- ----- ----- -----
//     GPU sampler part
// ----- ----- ----- ----- -----

// -- samplers
// >= 3.2
XMACRO(DELETESAMPLERS,    DeleteSamplers)
XMACRO(BINDSAMPLER,       BindSampler)
XMACRO(SAMPLERPARAMETERF, SamplerParameterf)
XMACRO(SAMPLERPARAMETERI, SamplerParameteri)
XMACRO(SAMPLERPARAMETERFV, SamplerParameterfv)
XMACRO(SAMPLERPARAMETERIV, SamplerParameteriv)
// >= 4.5
XMACRO(CREATESAMPLERS, CreateSamplers)

// ----- ----- ----- ----- -----
//     GPU texture part
// ----- ----- ----- ----- -----

// >= 2.0
XMACRO(DELETETEXTURES, DeleteTextures)
// >= 4.5
XMACRO(CREATETEXTURES,        CreateTextures)
XMACRO(TEXTURESTORAGE2D,      TextureStorage2D)
XMACRO(TEXTURESUBIMAGE2D,     TextureSubImage2D)
XMACRO(GENERATETEXTUREMIPMAP, GenerateTextureMipmap)
XMACRO(TEXTUREPARAMETERF,  TextureParameterf)
XMACRO(TEXTUREPARAMETERI,  TextureParameteri)
XMACRO(TEXTUREPARAMETERFV, TextureParameterfv)
XMACRO(TEXTUREPARAMETERIV, TextureParameteriv)
XMACRO(GETTEXTUREPARAMETERFV, GetTextureParameterfv)
XMACRO(GETTEXTUREPARAMETERIV, GetTextureParameteriv)
XMACRO(BINDTEXTUREUNIT,    BindTextureUnit)

// ----- ----- ----- ----- -----
//     GPU target part
// ----- ----- ----- ----- -----

// >= 3.0
XMACRO(DELETEFRAMEBUFFERS,  DeleteFramebuffers)
XMACRO(DELETERENDERBUFFERS, DeleteRenderbuffers)
// >= 4.5
XMACRO(CREATEFRAMEBUFFERS,           CreateFramebuffers)
XMACRO(NAMEDFRAMEBUFFERTEXTURE,      NamedFramebufferTexture)
XMACRO(CREATERENDERBUFFERS,          CreateRenderbuffers)
XMACRO(NAMEDRENDERBUFFERSTORAGE,     NamedRenderbufferStorage)
XMACRO(NAMEDFRAMEBUFFERRENDERBUFFER, NamedFramebufferRenderbuffer)
XMACRO(NAMEDFRAMEBUFFERREADBUFFER,   NamedFramebufferReadBuffer)
XMACRO(NAMEDFRAMEBUFFERDRAWBUFFER,   NamedFramebufferDrawBuffer)

// ----- ----- ----- ----- -----
//     GPU buffer part
// ----- ----- ----- ----- -----

// >= 2.0
XMACRO(GENBUFFERS,    GenBuffers)
XMACRO(DELETEBUFFERS, DeleteBuffers)
// >= 3.0
XMACRO(BINDBUFFERBASE,  BindBufferBase)
XMACRO(BINDBUFFERRANGE, BindBufferRange)
// >= 4.5
XMACRO(CREATEBUFFERS,      CreateBuffers)
XMACRO(NAMEDBUFFERSTORAGE, NamedBufferStorage)
XMACRO(NAMEDBUFFERSUBDATA, NamedBufferSubData)

// ----- ----- ----- ----- -----
//     GPU mesh part
// ----- ----- ----- ----- -----

// >= 3.0
XMACRO(DELETEVERTEXARRAYS, DeleteVertexArrays)
XMACRO(BINDVERTEXARRAY,    BindVertexArray)
// >= 4.5
XMACRO(CREATEVERTEXARRAYS,       CreateVertexArrays)
XMACRO(VERTEXARRAYELEMENTBUFFER, VertexArrayElementBuffer)
XMACRO(VERTEXARRAYVERTEXBUFFER,  VertexArrayVertexBuffer)
XMACRO(ENABLEVERTEXARRAYATTRIB,  EnableVertexArrayAttrib)
XMACRO(DISABLEVERTEXARRAYATTRIB, DisableVertexArrayAttrib)
XMACRO(VERTEXARRAYATTRIBBINDING, VertexArrayAttribBinding)
XMACRO(VERTEXARRAYATTRIBFORMAT,  VertexArrayAttribFormat)

// ----- ----- ----- ----- -----
//     GPU drawing part
// ----- ----- ----- ----- -----

// -- display
// >= 2.0
XMACRO(VIEWPORT,        Viewport)
XMACRO(BINDFRAMEBUFFER, BindFramebuffer)
//
XMACRO(DRAWELEMENTSINSTANCED, DrawElementsInstanced)
XMACRO(DRAWARRAYSINSTANCED,   DrawArraysInstanced)
XMACRO(CLEAR,  Clear)
XMACRO(FLUSH,  Flush)
XMACRO(FINISH, Finish)

#undef XMACRO
