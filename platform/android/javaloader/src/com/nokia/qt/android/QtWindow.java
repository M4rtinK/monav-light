package com.nokia.qt.android;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

public class QtWindow extends View implements QtWindowInterface
{
	private Bitmap bitmap=null;
	private int left, top, right, bottom;

	public QtWindow(Context context, int windowId, int l, int t, int r, int b)
	{
		super(context);
		setFocusable(true);
		setId(windowId);
		left=l;
		top=t;
		right=r;
		bottom=b;
		QtApplication.lockWindow();
		bitmap = Bitmap.createBitmap(right-left+1, bottom-top+1, Bitmap.Config.RGB_565);
        QtApplication.unlockWindow();			
	}
	
	@Override
	protected void onDraw(Canvas canvas)
	{
    	QtApplication.lockWindow();
		//canvas.drawBitmap(bitmap, canvas.getClipBounds(), canvas.getClipBounds(), null);
    	canvas.drawBitmap(bitmap, 0, 0, null);
        QtApplication.unlockWindow();
	}
	
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh)
	{
		Log.i(QtApplication.QtTAG, "onSizeChanged id:"+getId()+" w:"+w+" h:"+h+" oldw:"+oldw+"oldh:"+oldh+" reqw:"+(right-left)+"reqh"+(bottom -top));
		if ( w != right-left || h != bottom -top )
			layout(left, top, right, bottom);
		invalidate();
	}

	@Override
	protected void onWindowVisibilityChanged(int visibility)
	{
		Log.i(QtApplication.QtTAG, "id:"+getId()+" WindowVisibilityChanged:"+visibility);
		if (visibility==VISIBLE)
		{
	        QtApplication.lockWindow();
	        QtApplication.windowCreated(bitmap, getId());
	        QtApplication.unlockWindow();
	        bringToFront();
	        invalidate();
		}
		else if (visibility==GONE) 
		{
	        QtApplication.lockWindow();
	        QtApplication.windowDestroyed(getId());
	        QtApplication.unlockWindow();
		}
		super.onWindowVisibilityChanged(visibility);
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

	@Override
	public int isOpenGl()
	{
		return 0;
	}

	@Override
	public void Resize(int l, int t, int r, int b)
	{
		left=l;
		top=t;
		right=r;
		bottom=b;
		QtApplication.lockWindow();
		bitmap = Bitmap.createBitmap(right-left+1, bottom-top+1, Bitmap.Config.RGB_565);
        QtApplication.windowChanged(bitmap, getId());
        QtApplication.unlockWindow();			
        bringToFront();
		layout(left, top, right, bottom);
		invalidate();
	}
}
