////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2017 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics/RenderTextureImplFBO.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/GLCheck.hpp>
#include <SFML/System/Err.hpp>


#ifdef __EMSCRIPTEN__
#undef GLEXT_glDeleteRenderbuffers
#undef GLEXT_glDeleteFramebuffers
#undef GLEXT_glGenFramebuffers
#undef GLEXT_glBindFramebuffer
#undef GLEXT_GL_FRAMEBUFFER
#undef GLEXT_glGenRenderbuffers
#undef GLEXT_glBindRenderbuffer
#undef GLEXT_GL_RENDERBUFFER
#undef GLEXT_glRenderbufferStorage
#undef GLEXT_GL_DEPTH_COMPONENT
#undef GLEXT_glFramebufferRenderbuffer
#undef GLEXT_GL_DEPTH_ATTACHMENT
#undef GLEXT_glFramebufferTexture2D
#undef GLEXT_GL_COLOR_ATTACHMENT0
#undef GLEXT_glCheckFramebufferStatus
#undef GLEXT_GL_FRAMEBUFFER_COMPLETE

#define GLEXT_glDeleteRenderbuffers glDeleteRenderbuffers
#define GLEXT_glDeleteFramebuffers glDeleteFramebuffers
#define GLEXT_glGenFramebuffers glGenFramebuffers
#define GLEXT_glBindFramebuffer glBindFramebuffer
#define GLEXT_GL_FRAMEBUFFER 0x8D40
#define GLEXT_glGenRenderbuffers glGenRenderbuffers
#define GLEXT_glBindRenderbuffer glBindRenderbuffer
#define GLEXT_GL_RENDERBUFFER 0x8D41
#define GLEXT_glRenderbufferStorage glRenderbufferStorage
#define GLEXT_GL_DEPTH_COMPONENT 0x1902
#define GLEXT_glFramebufferRenderbuffer glFramebufferRenderbuffer
#define GLEXT_GL_DEPTH_ATTACHMENT 0x8D00
#define GLEXT_glFramebufferTexture2D glFramebufferTexture2D
#define GLEXT_GL_COLOR_ATTACHMENT0 0x8CE0
#define GLEXT_glCheckFramebufferStatus glCheckFramebufferStatus
#define GLEXT_GL_FRAMEBUFFER_COMPLETE 0x8CD5

#define GL_APICALL extern "C"
#define GL_APIENTRY
// copied from SDL_opengles2_gl2.h
GL_APICALL void         GL_APIENTRY glDeleteRenderbuffers (GLsizei n, const GLuint* renderbuffers);
GL_APICALL void         GL_APIENTRY glDeleteFramebuffers (GLsizei n, const GLuint* framebuffers);
GL_APICALL void         GL_APIENTRY glGenFramebuffers (GLsizei n, GLuint* framebuffers);
GL_APICALL void         GL_APIENTRY glBindFramebuffer (GLenum target, GLuint framebuffer);
GL_APICALL void         GL_APIENTRY glGenRenderbuffers (GLsizei n, GLuint* renderbuffers);
GL_APICALL void         GL_APIENTRY glBindRenderbuffer (GLenum target, GLuint renderbuffer);
GL_APICALL void         GL_APIENTRY glRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
GL_APICALL void         GL_APIENTRY glFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
GL_APICALL void         GL_APIENTRY glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GL_APICALL GLenum       GL_APIENTRY glCheckFramebufferStatus (GLenum target);
#endif	// __EMSCRIPTEN__


namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
RenderTextureImplFBO::RenderTextureImplFBO() :
m_context    (NULL),
m_frameBuffer(0),
m_depthBuffer(0)
{

}


////////////////////////////////////////////////////////////
RenderTextureImplFBO::~RenderTextureImplFBO()
{
    m_context->setActive(true);

    // Destroy the depth buffer
    if (m_depthBuffer)
    {
        GLuint depthBuffer = static_cast<GLuint>(m_depthBuffer);
        glCheck(GLEXT_glDeleteRenderbuffers(1, &depthBuffer));
    }

    // Destroy the frame buffer
    if (m_frameBuffer)
    {
        GLuint frameBuffer = static_cast<GLuint>(m_frameBuffer);
        glCheck(GLEXT_glDeleteFramebuffers(1, &frameBuffer));
    }

    // Delete the context
    delete m_context;
}


////////////////////////////////////////////////////////////
bool RenderTextureImplFBO::isAvailable()
{
    TransientContextLock lock;

    // Make sure that extensions are initialized
    priv::ensureExtensionsInit();

#ifdef __EMSCRIPTEN__
	return true;
#else
    return GLEXT_framebuffer_object != 0;
#endif
}


////////////////////////////////////////////////////////////
bool RenderTextureImplFBO::create(unsigned int width, unsigned int height, unsigned int textureId, bool depthBuffer)
{
    // Create the context
    m_context = new Context;

    // Create the framebuffer object
    GLuint frameBuffer = 0;
    glCheck(GLEXT_glGenFramebuffers(1, &frameBuffer));
    m_frameBuffer = static_cast<unsigned int>(frameBuffer);
    if (!m_frameBuffer)
    {
        err() << "Impossible to create render texture (failed to create the frame buffer object)" << std::endl;
        return false;
    }
    glCheck(GLEXT_glBindFramebuffer(GLEXT_GL_FRAMEBUFFER, m_frameBuffer));

    // Create the depth buffer if requested
    if (depthBuffer)
    {
        GLuint depth = 0;
        glCheck(GLEXT_glGenRenderbuffers(1, &depth));
        m_depthBuffer = static_cast<unsigned int>(depth);
        if (!m_depthBuffer)
        {
            err() << "Impossible to create render texture (failed to create the attached depth buffer)" << std::endl;
            return false;
        }
        glCheck(GLEXT_glBindRenderbuffer(GLEXT_GL_RENDERBUFFER, m_depthBuffer));
        glCheck(GLEXT_glRenderbufferStorage(GLEXT_GL_RENDERBUFFER, GLEXT_GL_DEPTH_COMPONENT, width, height));
        glCheck(GLEXT_glFramebufferRenderbuffer(GLEXT_GL_FRAMEBUFFER, GLEXT_GL_DEPTH_ATTACHMENT, GLEXT_GL_RENDERBUFFER, m_depthBuffer));
    }

    // Link the texture to the frame buffer
    glCheck(GLEXT_glFramebufferTexture2D(GLEXT_GL_FRAMEBUFFER, GLEXT_GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0));

    // A final check, just to be sure...
    GLenum status;
    glCheck(status = GLEXT_glCheckFramebufferStatus(GLEXT_GL_FRAMEBUFFER));
    if (status != GLEXT_GL_FRAMEBUFFER_COMPLETE)
    {
        glCheck(GLEXT_glBindFramebuffer(GLEXT_GL_FRAMEBUFFER, 0));
        err() << "Impossible to create render texture (failed to link the target texture to the frame buffer)" << std::endl;
        return false;
    }

    return true;
}


////////////////////////////////////////////////////////////
bool RenderTextureImplFBO::activate(bool active)
{
#ifndef __EMSCRIPTEN__
	return m_context->setActive(active);
#else
	// no support for m_depthBuffer yet
	glCheck(GLEXT_glBindFramebuffer(GLEXT_GL_FRAMEBUFFER, (active ? m_frameBuffer : 0)));
	return true;
#endif
}


////////////////////////////////////////////////////////////
void RenderTextureImplFBO::updateTexture(unsigned int)
{
    glCheck(glFlush());
}

} // namespace priv

} // namespace sf
