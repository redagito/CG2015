#include "RendererDebug.h"

#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <spdlog/spdlog.h>

#include "graphics/renderer/core/RendererCoreConfig.h"

namespace
{
void GLAD_API_PTR debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                             GLsizei, const GLchar* message, const void*)
{
    const bool fatal=type==GL_DEBUG_TYPE_ERROR || type==GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR || severity==GL_DEBUG_SEVERITY_HIGH;
    try {
        spdlog::log(spdlog::source_loc{__FILE__,__LINE__,__func__},fatal?spdlog::level::err:spdlog::level::warn,
            "OpenGL [source={:#x} type={:#x} severity={:#x} id={}]: {}",source,type,severity,id,message);
        spdlog::default_logger()->flush();
    } catch(...) {
        // Logging failure must never unwind through the driver's C callback.
        std::fprintf(stderr,"OpenGL [source=0x%x type=0x%x severity=0x%x id=%u]: %s\n",source,type,severity,id,message);
        std::fflush(stderr);
    }
    if (fatal)
    {
        // Never throw across the driver's C callback boundary.
        std::abort();
    }
}
}

bool initializeOpenGLDebug()
{
    GLint major = 0, minor = 0, flags = 0, profile = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
    if (major < 4 || (major == 4 && minor < 6) ||
        !(flags & GL_CONTEXT_FLAG_DEBUG_BIT) || !(profile & GL_CONTEXT_CORE_PROFILE_BIT) ||
        !glDebugMessageCallback || !glDebugMessageControl)
    {
        SPDLOG_ERROR("OpenGL 4.6 core with a debug context is required.");
        return false;
    }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
                          0, nullptr, GL_FALSE);
    SPDLOG_INFO("OpenGL {} | {} | {} | synchronous debug enabled",
        reinterpret_cast<const char*>(glGetString(GL_VERSION)),reinterpret_cast<const char*>(glGetString(GL_VENDOR)),
        reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    return true;
}

bool hasGLError(std::string& errorText)
{
    GLenum code;
    bool fail = false;
    std::stringstream error;

    code = glGetError();
    // Build error string
    while (code != GL_NO_ERROR)
    {
        if (code == GL_INVALID_ENUM)
        {
            error << "An unacceptable value is specified for an enumerated argument. ";
            fail = true;
        }
        else if (code == GL_INVALID_VALUE)
        {
            error << "A numeric argument is out of range. ";
            fail = true;
        }
        else if (code == GL_INVALID_OPERATION)
        {
            error << "The specified operation is not allowed in the current state. ";
            fail = true;
        }
        else if (code == GL_INVALID_FRAMEBUFFER_OPERATION)
        {
            error << "The framebuffer object is not complete. ";
            fail = true;
        }
        else if (code == GL_OUT_OF_MEMORY)
        {
            error << "There is not enough memory left to execute the command. ";
            fail = true;
        }
        code = glGetError();
        if (code != GL_NO_ERROR)
        {
            error << std::endl;
        }
    }

    if (fail)
    {
        errorText = error.str();
    }
    return fail;
}

void validateFramebufferFeedback()
{
#ifdef CG2015_VALIDATE_PASSES
    GLint fbo=0,program=0; glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&fbo); glGetIntegerv(GL_CURRENT_PROGRAM,&program);
    if(!fbo || !program) return;
    std::vector<GLuint> writes;
    GLint count=0; glGetIntegerv(GL_MAX_DRAW_BUFFERS,&count);
    for(int i=0;i<count;++i) {
        GLint attachment=0; glGetIntegerv(GL_DRAW_BUFFER0+i,&attachment); if(attachment==GL_NONE) continue;
        GLint type=0,id=0; glGetNamedFramebufferAttachmentParameteriv(fbo,attachment,GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,&type);
        if(type==GL_TEXTURE) {glGetNamedFramebufferAttachmentParameteriv(fbo,attachment,GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,&id); writes.push_back(id);}
    }
    if(glIsEnabled(GL_DEPTH_TEST)) { GLboolean mask=GL_FALSE; glGetBooleanv(GL_DEPTH_WRITEMASK,&mask); if(mask) { GLint type=0,id=0; glGetNamedFramebufferAttachmentParameteriv(fbo,GL_DEPTH_ATTACHMENT,GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,&type); if(type==GL_TEXTURE) {glGetNamedFramebufferAttachmentParameteriv(fbo,GL_DEPTH_ATTACHMENT,GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,&id); writes.push_back(id);} } }
    GLint uniforms=0,maxName=0,active=0; glGetProgramiv(program,GL_ACTIVE_UNIFORMS,&uniforms); glGetProgramiv(program,GL_ACTIVE_UNIFORM_MAX_LENGTH,&maxName); glGetIntegerv(GL_ACTIVE_TEXTURE,&active);
    std::vector<char> name(static_cast<size_t>(maxName)+1);
    for(GLint i=0;i<uniforms;++i) {
        GLsizei length; GLint size; GLenum type; glGetActiveUniform(program,i,maxName,&length,&size,&type,name.data());
        GLenum binding=0;
        if(type==GL_SAMPLER_2D || type==GL_SAMPLER_2D_SHADOW) binding=GL_TEXTURE_BINDING_2D;
        if(type==GL_SAMPLER_CUBE || type==GL_SAMPLER_CUBE_SHADOW) binding=GL_TEXTURE_BINDING_CUBE_MAP;
        if(!binding) continue;
        const GLint location=glGetUniformLocation(program,name.data());
        for(GLint element=0;element<size;++element) {GLint unit=0,texture=0; glGetUniformiv(program,location+element,&unit); glActiveTexture(GL_TEXTURE0+unit); glGetIntegerv(binding,&texture);
            if(texture && std::find(writes.begin(),writes.end(),static_cast<GLuint>(texture))!=writes.end()) {glActiveTexture(active); throw std::runtime_error("Framebuffer feedback: program " + std::to_string(program) + " samples draw attachment through " + name.data());}
        }
    }
    glActiveTexture(active);
#endif
}
