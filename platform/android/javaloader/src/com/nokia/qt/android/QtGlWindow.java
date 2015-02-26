package com.nokia.qt.android;

import android.content.Context;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class QtGlWindow extends SurfaceView implements SurfaceHolder.Callback, QtWindowInterface  {
	private int oldx, oldy;
	private int left, top, right, bottom;
	
	public QtGlWindow(Context context, int windowId, int l, int t, int r, int b)
	{
		super(context);
		Log.i(QtApplication.QtTAG,"QtWindow "+windowId+" Left:"+l+" Top:"+t+" Right:"+r+" Bottom:"+b);
		setFocusable(true);
		getHolder().addCallback(this);
		setId(windowId);
		left=l;
		top=t;
		right=r;
		bottom=b;
	}
	
    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
        QtApplication.lockWindow();
        QtApplication.getEgl().createSurface(holder, getId());
        QtApplication.windowCreated(null, getId());
        QtApplication.unlockWindow();
        layout(left, top, right, bottom);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
    	QtApplication.lockWindow();
    	QtApplication.getEgl().createSurface(holder, getId());
        QtApplication.windowChanged(null, getId());
        QtApplication.unlockWindow();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
    	QtApplication.lockWindow();
        QtApplication.getEgl().destroySurface(getId());
        QtApplication.windowDestroyed(getId());
        QtApplication.unlockWindow();
    }

    private int getAction(int index, MotionEvent event)
    {
    	int action=event.getAction();
		if (action == MotionEvent.ACTION_MOVE)
		{
			int hsz=event.getHistorySize();
			if (hsz>0)
			{
				if (Math.abs(event.getX(index)-event.getHistoricalX(index, hsz-1))>1||
						Math.abs(event.getY(index)-event.getHistoricalY(index, hsz-1))>1)
					return 1;
				else
					return 2;
			}
			return 1;
		}

    	switch(index)
		{
		case 0:
			if (action == MotionEvent.ACTION_DOWN || 
					action == MotionEvent.ACTION_POINTER_1_DOWN)
				return 0;
			if (action == MotionEvent.ACTION_UP || 
					action == MotionEvent.ACTION_POINTER_1_UP)
				return 3;
			break;
		case 1:
			if (action == MotionEvent.ACTION_POINTER_2_DOWN ||
					action == MotionEvent.ACTION_POINTER_DOWN)
				return 0;
			if (action == MotionEvent.ACTION_POINTER_2_UP ||
					action == MotionEvent.ACTION_POINTER_UP)
				return 3;
			break;
		case 2:
			if (action == MotionEvent.ACTION_POINTER_3_DOWN ||
					action == MotionEvent.ACTION_POINTER_DOWN)
				return 0;
			if (action == MotionEvent.ACTION_POINTER_3_UP ||
					action == MotionEvent.ACTION_POINTER_UP)
				return 3;
			break;
		}
		return 2;
    }

    public void sendTouchEvents(MotionEvent event)
    {
		QtApplication.touchBegin(getId());

		for (int i=0;i<event.getPointerCount();i++)
			QtApplication.touchAdd(getId(),event.getPointerId(i), getAction(i, event), i==0,
					(int)event.getX(i), (int)event.getY(i), event.getSize(i),
					event.getPressure(i));

		switch(event.getAction())
		{
		case MotionEvent.ACTION_DOWN:
			QtApplication.touchEnd(getId(),0);
			break;
		case MotionEvent.ACTION_UP:
			QtApplication.touchEnd(getId(),2);
			break;
		default:
			QtApplication.touchEnd(getId(),1);
		}
    }

	@Override
	public boolean onTouchEvent(MotionEvent event)
	{
		sendTouchEvents(event);
		switch (event.getAction())
		{
		case MotionEvent.ACTION_UP:
			QtApplication.mouseUp(getId(),(int) event.getX(), (int) event.getY());
			return true;

		case MotionEvent.ACTION_DOWN:
			QtApplication.mouseDown(getId(),(int) event.getX(), (int) event.getY());
			oldx = (int) event.getX();
			oldy = (int) event.getY();
			return true;

		case MotionEvent.ACTION_MOVE:
			int dx = (int) (event.getX() - oldx);
			int dy = (int) (event.getY() - oldy);
			if (Math.abs(dx) > 5 || Math.abs(dy) > 5)
			{
				QtApplication.mouseMove(getId(),(int) event.getX(), (int) event.getY());
				oldx = (int) event.getX();
				oldy = (int) event.getY();
			}
			return true;
		}
		return true;
	}

	@Override
	public boolean onTrackballEvent(MotionEvent event)
	{
		switch (event.getAction())
		{
		case MotionEvent.ACTION_UP:
			QtApplication.mouseUp(getId(),(int) event.getX(), (int) event.getY());
			return true;

		case MotionEvent.ACTION_DOWN:
			QtApplication.mouseDown(getId(),(int) event.getX(), (int) event.getY());
			oldx = (int) event.getX();
			oldy = (int) event.getY();
			return true;

		case MotionEvent.ACTION_MOVE:
			int dx = (int) (event.getX() - oldx);
			int dy = (int) (event.getY() - oldy);
			if (Math.abs(dx) > 5 || Math.abs(dy) > 5)
			{
				QtApplication.mouseMove(getId(),(int) event.getX(), (int) event.getY());
				oldx = (int) event.getX();
				oldy = (int) event.getY();
			}
			return true;
		}
		return super.onTrackballEvent(event);
	}

	@Override
	public int isOpenGl() {
		return 1;
	}
	
	@Override
	public void Resize(int l, int t, int r, int b)
	{
		left=l;
		top=t;
		right=r;
		bottom=b;
		QtApplication.lockWindow();
        QtApplication.windowChanged(null, getId());
        QtApplication.unlockWindow();
		layout(left, top, right, bottom);
	}
}
