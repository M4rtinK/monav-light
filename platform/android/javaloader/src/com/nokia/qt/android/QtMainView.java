package com.nokia.qt.android;

import android.app.Activity;
import android.content.Context;
import android.util.DisplayMetrics;
import android.util.Log;
import android.widget.RelativeLayout;

public class QtMainView extends RelativeLayout
{
	public QtMainView(Context context)
	{
		super(context);
	}

	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh)
	{
		Log.i(QtApplication.QtTAG, "Desktop onSizeChanged:"+w+","+h);
		DisplayMetrics metrics = new DisplayMetrics();
		((Activity) getContext()).getWindowManager().getDefaultDisplay().getMetrics(metrics);
		QtApplication.setDisplayMetrics(metrics.widthPixels,
				metrics.heightPixels, w, h, metrics.xdpi, metrics.ydpi);
		super.onSizeChanged(w, h, oldw, oldh);
	}
}
