package com.nokia.qt.android;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class QtSurface extends SurfaceView implements SurfaceHolder.Callback
{
	private Bitmap mBitmap=null;
	
	public QtSurface(Context context)
	{
		super(context);
		setFocusable(true);
		getHolder().addCallback(this);
		getHolder().setType(SurfaceHolder.SURFACE_TYPE_GPU);
		setId(-1);
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder)
	{
    	QtApplication.lockSurface();
    	mBitmap=Bitmap.createBitmap(getWidth(), getHeight(), Bitmap.Config.RGB_565);
		QtApplication.setSurface(mBitmap);
        QtApplication.unlockSurface();
		QtApplication.updateWindow(-1);
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
	{
		Log.i(QtApplication.QtTAG,"surfaceChanged: "+width+","+height);
    	QtApplication.lockSurface();
    	mBitmap=Bitmap.createBitmap(width, height, Bitmap.Config.RGB_565);
		QtApplication.setSurface(mBitmap);
		DisplayMetrics metrics = new DisplayMetrics();
		((Activity) getContext()).getWindowManager().getDefaultDisplay().getMetrics(metrics);
		QtApplication.setDisplayMetrics(metrics.widthPixels, metrics.heightPixels,
										width, height, metrics.xdpi, metrics.ydpi);
		QtApplication.m_surface=this;
        QtApplication.unlockSurface();
		QtApplication.updateWindow(-1);
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder)
	{
		Log.i(QtApplication.QtTAG,"surfaceDestroyed ");
    	QtApplication.lockSurface();
		QtApplication.destroySurface();
		QtApplication.m_surface=null;
        QtApplication.unlockSurface();
	}

	public void drawBitmap(Rect rect)
	{
		QtApplication.lockSurface();
		if (null!=mBitmap)
		{
			try
			{
				Canvas cv=getHolder().lockCanvas(rect);
				cv.drawBitmap(mBitmap, rect, rect, null);
				getHolder().unlockCanvasAndPost(cv);
			}
			catch (Exception e)
			{
				Log.e(QtApplication.QtTAG, "Can't create main activity", e);
			}
		}
    	QtApplication.unlockSurface();
    }
	
	@Override
	public boolean onTouchEvent(MotionEvent event)
	{
		QtApplication.sendTouchEvent(event, getId());
		return true;
	}

	@Override
	public boolean onTrackballEvent(MotionEvent event)
	{
		QtApplication.sendTrackballEvent(event, getId());
		return true;
	}
}
