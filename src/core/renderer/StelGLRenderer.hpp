#ifndef _STELGLRENDERER_HPP_
#define _STELGLRENDERER_HPP_

#include <QGLFramebufferObject>
#include <QImage>
#include <QPainter>
#include <QSize>

#include "StelRenderer.hpp"

#include "StelGLUtilityFunctions.hpp"
#include "StelVertexBuffer.hpp"
#include "StelTestQGL2VertexBufferBackend.hpp"


//TODO At the moment, we're always using FBOs when supported.
//     We need a config file for Renderer implementation to disable it.

//TODO move QGL FBO (and maybe QPainter?) code to QGLRenderer

//! Base class for OpenGL based renderers.
//!
class StelGLRenderer : public StelRenderer
{
public:
	//! Construct a StelGL2Renderer.
	StelGLRenderer()
		: fboSupported(false)
		, fboDisabled(false)
		, sceneSize(QSize())
		, frontBuffer(NULL)
		, backBuffer(NULL)
		, backBufferPainter(NULL)
		, defaultPainter(NULL)
		, drawing(false)
		
	{
	}
	
	virtual ~StelGLRenderer()
	{
		if(NULL != frontBuffer)
		{
			delete frontBuffer;
			frontBuffer = NULL;
		}
		if(NULL != backBuffer)
		{
			delete backBuffer;
			backBuffer = NULL;
		}
	}
	
	virtual bool init()
	{
		fboSupported = QGLFramebufferObject::hasOpenGLFramebufferObjects();
		if (!useFBO())
		{
			qWarning() << "OpenGL framebuffer objects are disabled or not supported. "
			              "Can't use Viewport effects.";
		}
		return true;
	}
	
	virtual void enablePainting()
	{
		enablePainting(defaultPainter);
	}
	
	virtual void viewportHasBeenResized(QSize size)
	{
		invariant();
		//Can't check this in invariant because Renderer is initialized before 
		//AppGraphicsWidget sets its viewport size
		Q_ASSERT_X(size.isValid(), "StelQGLRenderer::viewportHasBeenResized",
		           "Invalid scene size");
		sceneSize = size;
		//We'll need FBOs of different size so get rid of the current FBOs.
		if (NULL != backBuffer)
		{
			delete backBuffer;
			backBuffer = NULL;
		}
		if (NULL != frontBuffer)
		{
			delete frontBuffer;
			frontBuffer = NULL;
		}
		invariant();
	}
	
	virtual void setDefaultPainter(QPainter* painter)
	{
		defaultPainter = painter;
	}
	
	virtual void startDrawing()
	{
		invariant();
		
		makeGLContextCurrent();
		
		drawing = true;
		if (useFBO())
		{
			//Draw to backBuffer.
			initFBO();
			backBuffer->bind();
			backBufferPainter = new QPainter(backBuffer);
			enablePainting(backBufferPainter);
		}
		else
		{
			enablePainting(defaultPainter);
		}
		invariant();
	}
	
	virtual void suspendDrawing()
	{
		invariant();
		disablePainting();
		
		if (useFBO())
		{
			//Release the backbuffer but don't swap it yet - we'll continue the drawing later.
			delete backBufferPainter;
			backBufferPainter = NULL;
			
			backBuffer->release();
		}
		drawing = false;
		invariant();
	}
	
	virtual void finishDrawing()
	{
		invariant();
		disablePainting();
		
		if (useFBO())
		{
			//Release the backbuffer and swap it to front.
			delete backBufferPainter;
			backBufferPainter = NULL;
			
			backBuffer->release();
			swapBuffersFBO();
		}
		drawing = false;
		invariant();
	}
	
	virtual void drawWindow(StelViewportEffect* effect)
	{
		invariant();

		//Warn about any GL errors.
		checkGLErrors();
		
		//Effects are ignored when FBO is not supported.
		//That might be changed for some GPUs, but it might not be worth the effort.
		
		//Put the result of drawing to the FBO on the screen, applying an effect.
		if (useFBO())
		{
			Q_ASSERT_X(!backBuffer->isBound() && !frontBuffer->isBound(),
			           "StelGL2Renderer::drawWindow", 
			           "Framebuffer objects loadweren't released before drawing the result");
			enablePainting(defaultPainter);
			
			effect->paintViewportBuffer(frontBuffer, this);
			disablePainting();
		}
		invariant();
	}
	
protected:
	//! Make Stellarium GL context the currently used GL context. Call this before GL calls.
	virtual void makeGLContextCurrent() = 0;
	
protected:
	//! Enable painting, using specified painter.
	virtual void enablePainting(QPainter* painter) = 0;
	
	//! Asserts that we're in a valid state.
	//!
	//! Overriding methods should also call StelGLRenderer::invariant().
	virtual void invariant() const
	{
		const bool fbo = useFBO();
		Q_ASSERT_X(NULL == backBuffer || fbo, "StelQGLRenderer::invariant",
		           "We have a backbuffer even though we're not using FBO");
		Q_ASSERT_X(NULL == frontBuffer || fbo, "StelQGLRenderer::invariant",
		           "We have a frontbuffer even though we're not using FBO");
		Q_ASSERT_X(NULL == backBufferPainter || fbo, "StelQGLRenderer::invariant",
		           "We have a backbuffer painter even though we're not using FBO");
		Q_ASSERT_X(drawing && fbo ? backBuffer != NULL : true,
		           "StelQGLRenderer::invariant",
		           "We're drawing and using FBOs, but the backBuffer is NULL");
		Q_ASSERT_X(drawing && fbo ? frontBuffer != NULL : true,
		           "StelQGLRenderer::invariant",
		           "We're drawing and using FBOs, but the frontBuffer is NULL");
		Q_ASSERT_X(drawing && fbo ? backBufferPainter != NULL : true,
		           "StelQGLRenderer::invariant",
		           "We're drawing and using FBOs, but the backBufferPainter is NULL");
	}

	//! Check for any OpenGL errors. Useful for detecting incorrect GL code.
	void checkGLErrors() const
	{
		const GLenum glError = glGetError();
		if(glError == GL_NO_ERROR) {return;}

		qWarning() << "OpenGL error detected: " << glErrorToString(glError);
	}
	
private:
	//! Are frame buffer objects supported on this system?
	bool fboSupported;
	
	//! Disable frame buffer objects even if supported?
	bool fboDisabled;
	
	//! Graphics scene size.
	QSize sceneSize;
	
	//! Frontbuffer (i.e. displayed at the moment) frame buffer object, when using FBOs.
	class QGLFramebufferObject* frontBuffer;
	
	//! Backbuffer (i.e. drawn to at the moment) frame buffer object, when using FBOs.
	class QGLFramebufferObject* backBuffer;
	
	//! Painter to the FBO we're drawing to, when using FBOs.
	QPainter* backBufferPainter;
	
	//! Painter we're using when not drawing to an FBO. 
	//!
	//! If NULL, we use a painter provided by GLProvider, which paints to the GL widget.
	//! This is the case at program startup.
	QPainter* defaultPainter;
	
	//! Are we in the middle of drawing?
	bool drawing;
	
	//! Are we using framebuffer objects?
	bool useFBO() const
	{
		return fboSupported && !fboDisabled;
	}
	
	//! Initialize the frame buffer objects.
	void initFBO()
	{
		Q_ASSERT_X(useFBO(), "StelGL2Renderer::initFBO", "We're not using FBO");
		if (NULL == backBuffer)
		{
			Q_ASSERT_X(NULL == frontBuffer, "StelGL2Renderer::initFBO", 
			           "frontBuffer is not null even though backBuffer is");
			backBuffer = new QGLFramebufferObject(sceneSize,
			                                      QGLFramebufferObject::CombinedDepthStencil);
			frontBuffer = new QGLFramebufferObject(sceneSize,
			                                       QGLFramebufferObject::CombinedDepthStencil);
			Q_ASSERT_X(backBuffer->isValid() && frontBuffer->isValid(),
			           "StelGL2Renderer::initFBO", "Framebuffer objects failed to initialize");
		}
	}
	
	//! Swap front and back buffers, when using FBO.
	void swapBuffersFBO()
	{
		Q_ASSERT_X(useFBO(), "StelGL2Renderer::swapBuffersFBO", "We're not using FBO");
		QGLFramebufferObject* tmp = backBuffer;
		backBuffer = frontBuffer;
		frontBuffer = tmp;
	}
};

#endif // _STELGLRENDERER_HPP_
