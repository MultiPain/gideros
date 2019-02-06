/*
 * metalShaderProgram.cpp
 *
 *  Created on: 5 juin 2015
 *      Author: Nicolas
 */

#include "glog.h"
#include <set>
#include "metalShaders.h"

#define encoder ((metalShaderEngine *)ShaderEngine::Engine)->encoder

class metalShaderBufferCache : public ShaderBufferCache {
public:
	id<MTLBuffer> VBO;
	int keptCounter;
	metalShaderBufferCache()
	{
		VBO = nil;
		keptCounter=0;
		if (!allVBO)
			allVBO=new std::set<metalShaderBufferCache*>();
		allVBO->insert(this);
	}
	virtual ~metalShaderBufferCache()
	{
		if (VBO)
            [VBO release];
		allVBO->erase(this);
		if (allVBO->empty())
		{
			delete allVBO;
			allVBO=NULL;
		}
	}
	void recreate()
	{
		if (VBO)
            [VBO release];
		VBO=nil;
	}
	bool valid()
	{
		return (VBO!=nil);
	}
	static std::set<metalShaderBufferCache *> *allVBO;
};

std::set<metalShaderBufferCache *> *metalShaderBufferCache::allVBO=NULL;
int metalShaderProgram::vboFreeze=0;
int metalShaderProgram::vboUnfreeze=0;

static id<MTLLibrary> defaultLibrary=nil;
ShaderProgram *metalShaderProgram::stdBasic3=NULL;
ShaderProgram *metalShaderProgram::stdColor3=NULL;
ShaderProgram *metalShaderProgram::stdTexture3=NULL;
ShaderProgram *metalShaderProgram::stdTextureColor3=NULL;
ShaderProgram *metalShaderProgram::stdParticleT=NULL;
ShaderProgram *metalShaderProgram::stdParticlesT=NULL;

MTLBlendFactor metalShaderProgram::blendFactor2metal(ShaderEngine::BlendFactor blendFactor) {
    switch (blendFactor) {
        case ShaderEngine::ZERO:
            return MTLBlendFactorZero;
        case ShaderEngine::ONE:
            return MTLBlendFactorOne;
        case ShaderEngine::SRC_COLOR:
            return MTLBlendFactorSourceColor;
        case ShaderEngine::ONE_MINUS_SRC_COLOR:
            return MTLBlendFactorOneMinusSourceColor;
        case ShaderEngine::DST_COLOR:
            return MTLBlendFactorDestinationColor;
        case ShaderEngine::ONE_MINUS_DST_COLOR:
            return MTLBlendFactorOneMinusDestinationColor;
        case ShaderEngine::SRC_ALPHA:
            return MTLBlendFactorSourceAlpha;
        case ShaderEngine::ONE_MINUS_SRC_ALPHA:
            return MTLBlendFactorOneMinusSourceAlpha;
        case ShaderEngine::DST_ALPHA:
            return MTLBlendFactorDestinationAlpha;
        case ShaderEngine::ONE_MINUS_DST_ALPHA:
            return MTLBlendFactorOneMinusDestinationAlpha;
        case ShaderEngine::SRC_ALPHA_SATURATE:
            return MTLBlendFactorSourceAlphaSaturated;
        default:
            break;
    }
    
    return MTLBlendFactorZero;
}

id<MTLBuffer> getCachedVBO(ShaderBufferCache **cache,bool &modified, int isize) {
	if (!cache) return nil; //XXX: Could we check for VBO availability ?
	if (!*cache)
		*cache = new metalShaderBufferCache();
	metalShaderBufferCache *dc = static_cast<metalShaderBufferCache*> (*cache);
    if ((dc->valid())&&([dc->VBO length]<isize))
        dc->recreate();
	if (!dc->valid())
	{
            dc->VBO=[metalDevice newBufferWithLength:isize options:MTLResourceStorageModeShared];
            [dc->VBO retain];
			modified=true;
	}
	return dc->VBO;
}

ShaderProgram *metalShaderProgram::current = NULL;
std::vector<metalShaderProgram *> metalShaderProgram::shaders;

void metalShaderProgram::resetAll()
{
    current = NULL;
    resetAllUniforms();
    /*
	  for (std::vector<metalShaderProgram *>::iterator it = shaders.begin() ; it != shaders.end(); ++it)
		  (*it)->recreate();
	  if (metalShaderBufferCache::allVBO)
		  for (std::set<metalShaderBufferCache *>::iterator it = metalShaderBufferCache::allVBO->begin() ; it != metalShaderBufferCache::allVBO->end(); ++it)
			  (*it)->recreate();
     */
}

void metalShaderProgram::resetAllUniforms()
{
	  for (std::vector<metalShaderProgram *>::iterator it = shaders.begin() ; it != shaders.end(); ++it)
		  (*it)->resetUniforms();
}

bool metalShaderProgram::isValid()
{
	return mrpd.vertexFunction&&mrpd.fragmentFunction;
}

const char *metalShaderProgram::compilationLog()
{
	return errorLog.c_str();
}

void metalShaderProgram::deactivate() {
	current = NULL;
}

void metalShaderProgram::activate() {
    //Update uniforms
    if (uniformVmodified)
        [encoder() setVertexBytes:cbData length:cbsData atIndex:0];
    if (uniformFmodified)
        [encoder() setFragmentBytes:cbData length:cbsData atIndex:0];
    uniformVmodified=false;
    uniformFmodified=false;

	if (current == this)
		return;
    useProgram();
	if (current)
		current->deactivate();
	current = this;
}

void metalShaderProgram::useProgram() {
    MTLRenderPassDescriptor *rd=((metalShaderEngine *)ShaderEngine::Engine)->pass();
    int pkey=rd.colorAttachments[0].texture.pixelFormat;
    pkey|=(metalShaderEngine::curSFactor)<<8;
    pkey|=(metalShaderEngine::curSFactor)<<12;
    if (mrps[pkey]==nil) {
        MTLRenderPipelineColorAttachmentDescriptor *rba=mrpd.colorAttachments[0];
        rba.pixelFormat=rd.colorAttachments[0].texture.pixelFormat;
        rba.blendingEnabled = YES;
        rba.rgbBlendOperation = MTLBlendOperationAdd;
        rba.alphaBlendOperation = MTLBlendOperationAdd;
        rba.sourceRGBBlendFactor = blendFactor2metal(metalShaderEngine::curSFactor);
        rba.sourceAlphaBlendFactor = blendFactor2metal(metalShaderEngine::curSFactor);
        rba.destinationRGBBlendFactor = blendFactor2metal(metalShaderEngine::curDFactor);
        rba.destinationAlphaBlendFactor = blendFactor2metal(metalShaderEngine::curDFactor);
        mrpd.depthAttachmentPixelFormat=rd.depthAttachment.texture.pixelFormat;
        mrpd.stencilAttachmentPixelFormat=rd.stencilAttachment.texture.pixelFormat;

        mrps[pkey]=[metalDevice newRenderPipelineStateWithDescriptor:mrpd error:NULL];
        [mrps[pkey] retain];
    }

    [encoder() setRenderPipelineState:mrps[pkey]];
}

void metalShaderProgram::setData(int index, DataType type, int mult,
		const void *ptr, unsigned int count, bool modified,
		ShaderBufferCache **cache,int stride,int offset) {
    if ((index<0)||(index>=attributes.size())) return;
    
    assert(type==attributes[index].type);
    assert(mult==attributes[index].mult);
	int elmSize = 1;
	switch (type) {
	case DINT:
		elmSize = 4;
		break;
	case DBYTE:
		break;
	case DUBYTE:
		break;
	case DSHORT:
		elmSize = 2;
		break;
	case DUSHORT:
		elmSize = 2;
		break;
	case DFLOAT:
		elmSize = 4;
		break;
	}

    int isize=elmSize * count * mult;
    id<MTLBuffer> vbo=cache?getCachedVBO(cache,modified,isize):nil;
    if ((vbo==nil)&&(isize>4096))
        vbo=[metalDevice newBufferWithLength:isize options:MTLResourceStorageModeShared];
    if (vbo==nil) {
        [encoder() setVertexBytes:((char *)ptr)+offset length:isize atIndex:index+1];
    }
    else {
    if (modified||(!cache)) {
        memcpy([vbo contents],ptr,isize);
#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_1011
        [vbo didModifyRange:NSMakeRange(0,isize)];
#endif
#endif
    }
    
    [encoder() setVertexBuffer:vbo offset:offset atIndex:index+1];
    }
}

void metalShaderProgram::setConstant(int index, ConstantType type, int mult,
		const void *ptr) {
	if (!updateConstant(index, type, mult, ptr))
		return;
    if (uniforms[index].vertexShader)
        uniformVmodified=true;
    else
        uniformFmodified=true;
}

metalShaderProgram::metalShaderProgram(const char *vprogram,const char *fprogram,
                   const ConstantDesc *uniforms, const DataDesc *attributes,int attmap,int attstride) {
    
    if (defaultLibrary==nil)
        defaultLibrary=[metalDevice newDefaultLibrary];
    [defaultLibrary retain];
    
    mrpd=[[MTLRenderPipelineDescriptor alloc] init];
    mrpd.vertexFunction=[defaultLibrary newFunctionWithName:[NSString stringWithUTF8String:vprogram]];
    mrpd.fragmentFunction=[defaultLibrary newFunctionWithName:[NSString stringWithUTF8String:fprogram]];
    assert(mrpd.vertexFunction!=nil);
    assert(mrpd.fragmentFunction!=nil);
    setupStructures(uniforms, attributes,attmap,attstride);
    
    shaders.push_back(this);
}

metalShaderProgram::metalShaderProgram(const char *vshader, const char *fshader,int flags,
		const ConstantDesc *uniforms, const DataDesc *attributes) {
    mrpd=[[MTLRenderPipelineDescriptor alloc] init];
	bool fromCode=(flags&ShaderProgram::Flag_FromCode);
	char *vs = fromCode?(char *)vshader:(char *) LoadShaderFile(vshader, "metal", NULL);
	char *fs = fromCode?(char *)fshader:(char *) LoadShaderFile(fshader, "metal", NULL);
    if (vs&&fs) {
        NSError *err;
        id<MTLLibrary> l;
        l=[metalDevice newLibraryWithSource:[NSString stringWithUTF8String:vs] options:nil error:&err];
        if (err) {
            errorLog+="Error compiling vertex shader:\n";
            errorLog+=[[err description] UTF8String];
            errorLog+="\n";
            err=nil;
        }
        if (l)
            mrpd.vertexFunction=[l newFunctionWithName:@"main"];
        if (!mrpd.vertexFunction)
            errorLog+="No vertex shader function called 'main'\n";
        l=[metalDevice newLibraryWithSource:[NSString stringWithUTF8String:fs] options:nil error:&err];
        if (err) {
            errorLog+="Error compiling fragment shader:\n";
            errorLog+=[[err description] UTF8String];
            errorLog+="\n";
            err=nil;
        }
        if (l)
            mrpd.fragmentFunction=[l newFunctionWithName:@"main"];
        if (!mrpd.fragmentFunction)
            errorLog+="No fragment shader function called 'main'\n";
        setupStructures(uniforms, attributes,0xFFFF,0);
    }
	else if (vs==NULL)
		errorLog="Vertex shader code not found";
	else
		errorLog="Fragment shader code not found";
	if (!fromCode)
	{
		if (vs) free(vs);
		if (fs) free(fs);
	}
	shaders.push_back(this);
}

void metalShaderProgram::setupStructures(const ConstantDesc *uniforms, const DataDesc *attributes,int attmap,int attstride)
{
    mrpd.vertexDescriptor=[MTLVertexDescriptor vertexDescriptor];
    
	cbsData=0;
	while (!uniforms->name.empty()) {
		int usz = 0, ual = 4;
		ConstantDesc cd;
		cd = *(uniforms++);
		switch (cd.type) {
		case CTEXTURE:
			usz = 0;
			ual = 1;
			break;
		case CINT:
			usz = 4;
			ual = 4;
			break;
		case CFLOAT:
			usz = 4;
			ual = 4;
			break;
		case CFLOAT2:
			usz = 8;
			ual = 4;
			break;
		case CFLOAT3:
			usz = 12;
			ual = 4;
			break;
		case CFLOAT4:
			usz = 16;
			ual = 16;
			break;
		case CMATRIX:
			usz = 64;
			ual = 16;
			break;
		}
		if (cbsData & (ual - 1))
			cbsData += ual - (cbsData & (ual - 1));
		cd.offset = cbsData;
		cbsData += usz*cd.mult;
        if (usz>0)
            this->uniforms.push_back(cd);
	}
    if (cbsData&15) cbsData+=16-(cbsData&15); //Didn't see mention of it anywhere, but it seems like it is
	cbData = malloc(cbsData);
	for (int iu = 0; iu < this->uniforms.size(); iu++)
		this->uniforms[iu]._localPtr = ((char *) cbData)
				+ this->uniforms[iu].offset;
    
    int nattr=0;
	while (!attributes->name.empty()) {
        this->attributes.push_back(*attributes);
        if (attmap&(1<<nattr)) {
            MTLVertexAttributeDescriptor *vad=[[MTLVertexAttributeDescriptor new] autorelease];
            MTLVertexBufferLayoutDescriptor *vbd=[[MTLVertexBufferLayoutDescriptor new] autorelease];
            vad.bufferIndex=attributes->slot+1;
            vad.offset=attributes->offset;
            int sstep=4;
            switch (attributes->type) {
                case DFLOAT: {
                    switch (attributes->mult) {
                        case 1: vad.format=MTLVertexFormatFloat; break;
                        case 2: vad.format=MTLVertexFormatFloat2; break;
                        case 3: vad.format=MTLVertexFormatFloat3; break;
                        case 4: vad.format=MTLVertexFormatFloat4; break;
                        default: break;
                    }
                    break;
                }
                case DINT: {
                    switch (attributes->mult) {
                        case 1: vad.format=MTLVertexFormatInt; break;
                        case 2: vad.format=MTLVertexFormatInt2; break;
                        case 3: vad.format=MTLVertexFormatInt3; break;
                        case 4: vad.format=MTLVertexFormatInt4; break;
                        default: break;
                    }
                    break;
                }
                case DUBYTE: {
                    sstep=1;
                    switch (attributes->mult) {
                        case 1: vad.format=MTLVertexFormatUChar; break;
                        case 2: vad.format=MTLVertexFormatUChar2; break;
                        case 3: vad.format=MTLVertexFormatUChar3; break;
                        case 4: vad.format=MTLVertexFormatUChar4; break;
                        default: break;
                    }
                    break;
                }
                case DBYTE: {
                    sstep=1;
                    switch (attributes->mult) {
                        case 1: vad.format=MTLVertexFormatChar; break;
                        case 2: vad.format=MTLVertexFormatChar2; break;
                        case 3: vad.format=MTLVertexFormatChar3; break;
                        case 4: vad.format=MTLVertexFormatChar4; break;
                        default: break;
                    }
                    break;
                }
                case DUSHORT: {
                    sstep=2;
                    switch (attributes->mult) {
                        case 1: vad.format=MTLVertexFormatUShort; break;
                        case 2: vad.format=MTLVertexFormatUShort2; break;
                        case 3: vad.format=MTLVertexFormatUShort3; break;
                        case 4: vad.format=MTLVertexFormatUShort4; break;
                        default: break;
                    }
                    break;
                }
                case DSHORT: {
                    sstep=2;
                    switch (attributes->mult) {
                        case 1: vad.format=MTLVertexFormatShort; break;
                        case 2: vad.format=MTLVertexFormatShort2; break;
                        case 3: vad.format=MTLVertexFormatShort3; break;
                        case 4: vad.format=MTLVertexFormatShort4; break;
                        default: break;
                    }
                    break;
                }
            }
            mrpd.vertexDescriptor.attributes[nattr]=vad;
            vbd.stride=attstride?attstride:sstep*attributes->mult;
            mrpd.vertexDescriptor.layouts[nattr+1]=vbd;
        }
        attributes++;
        nattr++;
	}
    errorLog="";
    uniformVmodified=true;
    uniformFmodified=true;

    //Load program and setup pipeline state
	shaderInitialized();
}

void metalShaderProgram::recreate() {
    uniformVmodified=true;
    uniformFmodified=true;
}

void metalShaderProgram::resetUniforms() {
    uniformVmodified=true;
    uniformFmodified=true;
}

metalShaderProgram::~metalShaderProgram() {
	 for (std::vector<metalShaderProgram *>::iterator it = shaders.begin() ; it != shaders.end(); )
		if (*it==this)
			it=shaders.erase(it);
		else
			it++;

	if (current==this)
		deactivate();

    for (std::map<int,id<MTLRenderPipelineState>>::iterator it=mrps.begin(); it!=mrps.end(); ++it)
        [it->second release];

    [mrpd release];
	free(cbData);
}

void metalShaderProgram::drawArrays(ShapeType shape, int first,
		unsigned int count) {
	ShaderEngine::Engine->prepareDraw(this);
	activate();
	MTLPrimitiveType mode = MTLPrimitiveTypeTriangle;
	switch (shape) {
	case Point:
		mode = MTLPrimitiveTypePoint;
		break;
	case Lines:
		mode = MTLPrimitiveTypeLine;
		break;
	case LineLoop:
		mode = MTLPrimitiveTypeLineStrip;
		break;
	case Triangles:
		mode = MTLPrimitiveTypeTriangle;
		break;
	case TriangleFan:
		mode = MTLPrimitiveTypeTriangleStrip; //Unsupported
		break;
	case TriangleStrip:
		mode = MTLPrimitiveTypeTriangleStrip;
		break;
	}
    [encoder() drawPrimitives:mode vertexStart:first vertexCount:count];
}
void metalShaderProgram::drawElements(ShapeType shape, unsigned int count,
		DataType type, const void *indices, bool modified, ShaderBufferCache **cache,unsigned int first,unsigned int dcount) {
	ShaderEngine::Engine->prepareDraw(this);
	activate();

    MTLPrimitiveType mode = MTLPrimitiveTypeTriangle;
    switch (shape) {
        case Point:
            mode = MTLPrimitiveTypePoint;
            break;
        case Lines:
            mode = MTLPrimitiveTypeLine;
            break;
        case LineLoop:
            mode = MTLPrimitiveTypeLineStrip;
            break;
        case Triangles:
            mode = MTLPrimitiveTypeTriangle;
            break;
        case TriangleFan:
            mode = MTLPrimitiveTypeTriangleStrip; //Unsupported
            break;
        case TriangleStrip:
            mode = MTLPrimitiveTypeTriangleStrip;
            break;
    }

	MTLIndexType dtype = MTLIndexTypeUInt16;
	int elmSize=2;
	switch (type) {
        case DINT:
            dtype = MTLIndexTypeUInt32;
            elmSize=4;
		break;
        default:
            break;
	}
    int isize=elmSize * count;
    if (dcount==0) dcount=count;
    id<MTLBuffer> vbo=cache?getCachedVBO(cache,modified,isize):nil;
    if (vbo==nil)
        vbo=[metalDevice newBufferWithLength:isize options:MTLResourceStorageModeShared];
    if (modified||(!cache)) {
        memcpy([vbo contents],indices,isize);
#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
    #if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_1011
        [vbo didModifyRange:NSMakeRange(0,isize)];
    #endif
#endif
    }
    //XXX bufferoffset should be int32 aligned per Apple's docs, this can break rendering when indices are u16 and first is an odd vindex
    [encoder() drawIndexedPrimitives:mode
            indexCount:dcount indexType:dtype indexBuffer:vbo indexBufferOffset:first*elmSize];
}