package com.nokia.qt.android;

import java.util.HashMap;
import java.util.Map;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import android.util.Log;
import android.view.SurfaceHolder;

public class QtEgl {
	EGL10 mEgl = null;
	EGLDisplay mEglDisplay;
	EGLConfig mEglConfig;
	EGLContext mEglContext;
	EGLSurface mCurrentEglSurface;
    int mRedSize=5;
    int mGreenSize=6;
    int mBlueSize=5;
    int mAlphaSize=0;
    int mDepthSize=16;
    int mStencilSize=4;
    int mSamplesSize=0;
    int mSampleBuffersSize=0;
    
	Map<Integer, EGLSurface> mEglSurfaces = new HashMap<Integer, EGLSurface>();

	private EGLConfig chooseConfig(EGL10 egl, EGLDisplay display,
               EGLConfig[] configs, int[] aditionalConfigs)
	{
           EGLConfig closestConfig = null;
           int closestDistance = 1000;
           int[] value = new int[1];
           for(EGLConfig config : configs)
           {
        	   int distance = 0;
        	   for(int i=0;i<aditionalConfigs.length;i+=2)
        	   {
        		   if (EGL10.EGL_NONE == aditionalConfigs[i])
        			   break;
        		   if (egl.eglGetConfigAttrib(display, config, aditionalConfigs[i], value))
        			   distance += Math.abs(aditionalConfigs[i+1]- value[0]);
        	   }
               if (distance < closestDistance)
               {
                   closestDistance = distance;
                   closestConfig = config;
               }
           }
           return closestConfig;
       }

	public boolean initialize()
	{
		mEgl = (EGL10) EGLContext.getEGL();
		mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
		int[] version = new int[2];
		mEgl.eglInitialize(mEglDisplay, version);

		int[] niceToHaveConfigSpec = new int[] {EGL10.EGL_RED_SIZE, mRedSize,
		                        EGL10.EGL_GREEN_SIZE, mGreenSize,
		                        EGL10.EGL_BLUE_SIZE, mBlueSize,
		                        EGL10.EGL_ALPHA_SIZE, mAlphaSize,
		                        EGL10.EGL_DEPTH_SIZE, mDepthSize,
		                        EGL10.EGL_STENCIL_SIZE, mStencilSize,
		                        EGL10.EGL_SAMPLES, mSamplesSize,
		                        EGL10.EGL_SAMPLE_BUFFERS, mSampleBuffersSize,
		                        EGL10.EGL_NONE };

		int[] configSpec = new int[] {EGL10.EGL_RED_SIZE, mRedSize,
				                EGL10.EGL_GREEN_SIZE, mGreenSize,
				                EGL10.EGL_BLUE_SIZE, mBlueSize,
				                EGL10.EGL_NONE };

		int[] numConfigs = new int[1];
		mEgl.eglChooseConfig(mEglDisplay, configSpec, null, 0, numConfigs);

        if (numConfigs[0] <= 0) {
            throw new IllegalArgumentException(
                    "No configs match configSpec");
        }

        EGLConfig[] configs = new EGLConfig[numConfigs[0]];
        mEgl.eglChooseConfig(mEglDisplay, configSpec, configs, numConfigs[0], numConfigs);

		mEglConfig =chooseConfig(mEgl,mEglDisplay, configs, niceToHaveConfigSpec);
		mEglContext = mEgl.eglCreateContext(mEglDisplay, mEglConfig,EGL10.EGL_NO_CONTEXT, null);

		if (mEglContext == EGL10.EGL_NO_CONTEXT)
			throw new RuntimeException("CreateContext failed");

		mCurrentEglSurface = EGL10.EGL_NO_SURFACE;
		QtApplication.setEglObject(this);
		return true;
	}

	public boolean isInitialized()
	{
		return mEgl != null;
	}

	public void finish()
	{
		if (mEglContext != EGL10.EGL_NO_CONTEXT)
		{
			mEgl.eglDestroyContext(mEglDisplay, mEglContext);
			mEglContext = EGL10.EGL_NO_CONTEXT;
		}
		if (mEglDisplay != EGL10.EGL_NO_DISPLAY)
		{
			mEgl.eglTerminate(mEglDisplay);
			mEglDisplay = EGL10.EGL_NO_DISPLAY;
		}
		mEgl = null;
	}

	public boolean createSurface(SurfaceHolder holder, int surfaceId)
	{
		QtApplication.lockWindow();
		synchronized (this)
		{
			if (mEgl == null)
			{
				QtApplication.unlockWindow();
				return false;
			}

			if (mEglSurfaces.containsKey(surfaceId) && mEglSurfaces.get(surfaceId) == mCurrentEglSurface)
				mEgl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);

			if (mEglSurfaces.containsKey(surfaceId) && mEglSurfaces.get(surfaceId) != EGL10.EGL_NO_SURFACE)
				mEgl.eglDestroySurface(mEglDisplay, mEglSurfaces.get(surfaceId));

			EGLSurface eglSurface = mEgl.eglCreateWindowSurface(mEglDisplay, mEglConfig, holder, null);

			if (eglSurface == EGL10.EGL_NO_SURFACE)
			{
				QtApplication.unlockWindow();
				throw new RuntimeException("createWindowSurface failed");
			}

//			if (!mEgl.eglMakeCurrent(mEglDisplay, eglSurface, eglSurface, mEglContext))
//				throw new RuntimeException("eglMakeCurrent failed.");

			mEglSurfaces.put(surfaceId, eglSurface);
			QtApplication.unlockWindow();
			return true;
		}
	}

	public boolean makeCurrent(int surfaceId)
	{
		synchronized (this)
		{
			try
			{
				if (mEgl == null)
					return false;

				if (mEglSurfaces.containsKey(surfaceId) && mCurrentEglSurface != mEglSurfaces.get(surfaceId))
				{
					mEgl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
					if (mEgl.eglMakeCurrent(mEglDisplay, mEglSurfaces.get(surfaceId), mEglSurfaces.get(surfaceId), mEglContext))
						mCurrentEglSurface = mEglSurfaces.get(surfaceId);
					else
					{
						int error=mEgl.eglGetError();
						Log.i(QtApplication.QtTAG,"eglMakeCurrent failed with error code"+error);
						mCurrentEglSurface = EGL10.EGL_NO_SURFACE;
						return false;
					}
				}
				else
				{
					if (!mEglSurfaces.containsKey(surfaceId))
							Log.e(QtApplication.QtTAG,"Can't find surface "+surfaceId);
				}
				return true;
			}
			catch (Exception e)
			{
				return false;
			}
		}
	}

	public void doneCurrent()
	{
		synchronized (this)
		{
			mEgl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
			mCurrentEglSurface = EGL10.EGL_NO_SURFACE;
		}
	}

	public boolean swapBuffers(int surfaceId)
	{
		synchronized (this)
		{
			if (mEgl == null || !mEglSurfaces.containsKey(surfaceId) || mEglSurfaces.get(surfaceId) == EGL10.EGL_NO_SURFACE)
			{
				if (!mEglSurfaces.containsKey(surfaceId))
						Log.e(QtApplication.QtTAG,"Can't find surface "+surfaceId);
				return false;
			}
			boolean ret = mEgl.eglSwapBuffers(mEglDisplay, mEglSurfaces.get(surfaceId));
			if (!ret)
				mCurrentEglSurface = EGL10.EGL_NO_SURFACE;

			return ret;
		}
	}

	public void destroySurface(int surfaceId)
	{
		synchronized (this)
		{
			if (mEgl == null || !mEglSurfaces.containsKey(surfaceId) || mEglSurfaces.get(surfaceId) == EGL10.EGL_NO_SURFACE)
				return;

			mEgl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
			mEgl.eglDestroySurface(mEglDisplay, mEglSurfaces.get(surfaceId));

			if (mCurrentEglSurface == mEglSurfaces.get(surfaceId))
				mCurrentEglSurface = null;
			mEglSurfaces.remove(surfaceId);
		}
	}
}
