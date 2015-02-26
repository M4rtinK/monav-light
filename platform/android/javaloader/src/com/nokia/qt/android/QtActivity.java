package com.nokia.qt.android;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.method.MetaKeyKeyListener;
import android.util.Log;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

public class QtActivity extends Activity
{

	public enum QtLibrary {
        QtCore, QtNetwork, QtXml, QtXmlPatterns, QtScript, QtSql, QtGui, QtOpenGL, QtSvg, QtScriptTools, QtDeclarative, QtMultimedia, QtWebKit, QtAndroid_mw, QtAndroid_sw, QtLocation, QtAndroidBridge
    }
    private boolean singleWindow=true;
    private Object jniProxyObject = null;
    private boolean quitApp = true;
    private String appName = "calculator";
    private List<String> libraries = new ArrayList<String>();
    private boolean softwareKeyboardIsVisible=false;
    private long metaState;
    private int lastChar=0;
    private boolean fullScreen=false;
    private Process debuggerProcess=null;
    private static final int ProcessEvents = 1;
	private Handler mHandler = new Handler()
	{
		@Override
		public void handleMessage(Message msg)
		{
			switch(msg.what)
			{
				case ProcessEvents:
					QtApplication.processQtEvents();
					break;
			}
		}
	};

	public QtActivity()
    {
        // By default try to load all Qt libraries
        addQtLibrary(QtLibrary.QtCore);
        addQtLibrary(QtLibrary.QtXml);
        addQtLibrary(QtLibrary.QtNetwork);
//        addQtLibrary(QtLibrary.QtScript);
//        addQtLibrary(QtLibrary.QtSql);
        addQtLibrary(QtLibrary.QtGui);
        addQtLibrary(QtLibrary.QtOpenGL);
        addQtLibrary(QtLibrary.QtSvg);
//        addQtLibrary(QtLibrary.QtScriptTools);
//        addQtLibrary(QtLibrary.QtMultimedia);
//        addQtLibrary(QtLibrary.QtWebKit);
//        addQtLibrary(QtLibrary.QtXmlPatterns);
//        addQtLibrary(QtLibrary.QtDeclarative);
        addQtLibrary(QtLibrary.QtLocation);
        if (singleWindow)
        	addQtLibrary(QtLibrary.QtAndroid_sw);
        else
        	addQtLibrary(QtLibrary.QtAndroid_mw);
//        addQtLibrary(QtLibrary.QtAndroidBridge);
    }

    public void positionUpdate(double lat, double lon, double altitude, double bearing, double speed)
    {
        QtApplication.g_position_updated(lat, lon, altitude, bearing, speed);
    }

    public void setApplication(String app)
    {
        appName = app;
    }

    public void setFullScreen(boolean enterFullScreen)
    {
    	fullScreen=enterFullScreen;
    }
    
    public void setLibraries(List<String> libs)
    {
        libraries.clear();
        libraries.addAll(libs);
    }
    
    public void addLibrary(String lib)
    {
        if (!libraries.contains(lib))
            libraries.add(lib);
    }
    
    public void addQtLibrary(QtLibrary lib)
    {
        if (!libraries.contains(lib.toString()))
            libraries.add(lib.toString());
    }
    
    public void removeQtLibrary(QtLibrary lib)
    {
        int index = libraries.indexOf(lib.toString());
        if (index != -1)
            libraries.remove(index);
    }
    
    public void removeLibrary(String lib)
    {
        int index = libraries.indexOf(lib);
        if (index != -1)
            libraries.remove(index);
    }
    
    public void setJniProxyObject(Object jniProxyObject)
    {
        this.jniProxyObject = jniProxyObject;
    }
    
    public void showSoftwareKeyboard()
    {
    	softwareKeyboardIsVisible = true;
    	InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
    	imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, InputMethodManager.HIDE_IMPLICIT_ONLY );
    }

    public void hideSoftwareKeyboard()
    {
    	if (softwareKeyboardIsVisible)
    	{
    		InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
    		imm.toggleSoftInput(0, 0);
    	}
    	softwareKeyboardIsVisible = false;
    }
    
    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_MULTIPLE && 
            event.getCharacters() != null && 
            event.getCharacters().length() == 1 &&
            event.getKeyCode() == 0) {
            Log.i(QtApplication.QtTAG, "dispatchKeyEvent at MULTIPLE with one character: "+event.getCharacters());
            QtApplication.keyDown(0, event.getCharacters().charAt(0), event.getMetaState());
            QtApplication.keyUp(0, event.getCharacters().charAt(0), event.getMetaState());
        }
    
        return super.dispatchKeyEvent(event);
    }

   
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
    	metaState = MetaKeyKeyListener.handleKeyDown(metaState,
                keyCode, event);
        int c = event.getUnicodeChar(MetaKeyKeyListener.getMetaState(metaState));
        int lc=c;
        metaState = MetaKeyKeyListener.adjustMetaAfterKeypress(metaState);

        if ((c & KeyCharacterMap.COMBINING_ACCENT) != 0)
        {
            c = c & KeyCharacterMap.COMBINING_ACCENT_MASK;
            int composed = KeyEvent.getDeadChar(lastChar, c);
            c = composed;
        }
        lastChar = lc;
        if (keyCode != KeyEvent.KEYCODE_BACK)
        	QtApplication.keyDown(keyCode, c, event.getMetaState());
        return true;
    }

  
   
    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
    	metaState = MetaKeyKeyListener.handleKeyUp(metaState, keyCode, event);
        QtApplication.keyUp(keyCode, event.getUnicodeChar(), event.getMetaState());
        return true;
    }

    public void enterFullScreen()
    {
    	 getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,   
                 WindowManager.LayoutParams.FLAG_FULLSCREEN);
    	 fullScreen = true;
    }
    
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        if (fullScreen)
        	getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,   
        						WindowManager.LayoutParams.FLAG_FULLSCREEN);
        try
        {
            String pn=getPackageName();
            ApplicationInfo ai=getPackageManager().getApplicationInfo(pn, PackageManager.GET_META_DATA);
            String packagePath=ai.dataDir+"/";
            // FIXME: throws a NullReference exception.
            // int resourceId = ai.metaData.getInt("qt_libs_resource_id");
            // Log.i(QtApplication.QtTAG, packagePath +","+resourceId+","+getResources().getStringArray(resourceId).length+"++++++");
            if (ai.sharedLibraryFiles != null)
                for (int i=0;i<ai.sharedLibraryFiles.length;i++)
                        Log.i(QtApplication.QtTAG, ai.sharedLibraryFiles[i]);
            if (singleWindow)
            {
    			setContentView(new QtSurface(this));
            }
            else
            {
	            QtMainView view = new QtMainView(this);
	            setContentView(view);
	            QtApplication.setView(view);
            }
            QtApplication.setActivity(this);
            if (null == getLastNonConfigurationInstance())
            {
                QtApplication.loadLibraries(libraries);
                QtApplication.loadApplication(appName);
                if ((ai.flags&ApplicationInfo.FLAG_DEBUGGABLE) != 0 
                		&& getIntent().getExtras() != null 
                		&& getIntent().getExtras().containsKey("native_debug") 
                		&& getIntent().getExtras().getString("native_debug").equals("true"))
            	{
            		try
            		{
            			String gdbserverPath=getIntent().getExtras().containsKey("gdbserver_path")?getIntent().getExtras().getString("gdbserver_path"):packagePath+"lib/gdbserver ";
            			String socket=getIntent().getExtras().containsKey("gdbserver_socket")?getIntent().getExtras().getString("gdbserver_socket"):"+debug-socket";
            			debuggerProcess = Runtime.getRuntime().exec(gdbserverPath+socket+" --attach "+android.os.Process.myPid(),null, new File(packagePath));
            		}
            		catch (IOException ioe)
            	    {
            			Log.e(QtApplication.QtTAG,"Can't start debugging"+ioe.getMessage());
            	    }
            		catch (SecurityException se)
            	    {
            			Log.e(QtApplication.QtTAG,"Can't start debugging"+se.getMessage());
            	    }
            	}
                QtApplication.startApplication((jniProxyObject == null) ? this : jniProxyObject);
                Log.i(QtApplication.QtTAG, "onCreate");
            }
            quitApp = true;
        }
        catch (Exception e)
        {
            Log.e(QtApplication.QtTAG, "Can't create main activity", e);
        }
    }

    @Override
    protected void onPause()
    {
        if (!quitApp)
        {
            Log.i(QtApplication.QtTAG, "onPause");
            QtApplication.pauseQtApp();
        }
        super.onPause();
    }

    @Override
    protected void onResume()
    {
        Log.i(QtApplication.QtTAG, "onResume");
        QtApplication.resumeQtApp();
        super.onRestart();
    }

    @Override
    public Object onRetainNonConfigurationInstance()
    {
        super.onRetainNonConfigurationInstance();
        quitApp = false;
        return true;
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        if (quitApp)
        {
            Log.i(QtApplication.QtTAG, "onDestroy");
//            QtApplication.quitQtAndroidPlugin();
            if (debuggerProcess != null)
            	debuggerProcess.destroy();
            System.exit(0);
        }
        QtApplication.setActivity(null);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        Log.i(QtApplication.QtTAG, "onSaveInstanceState");
        super.onSaveInstanceState(outState);
        Log.i(QtApplication.QtTAG, "onSaveInstanceState");
        QtMainView view = QtApplication.getView();
        if (view != null)
        {
	        outState.putInt("Surfaces", view.getChildCount());
	        for (int i=0;i<view.getChildCount();i++)
	        {
	            QtWindow surface=(QtWindow) view.getChildAt(i);
	            Log.i(QtApplication.QtTAG,"id"+surface.getId()+","+surface.getLeft()+","+surface.getTop()+","+surface.getRight()+","+surface.getBottom());
	            int surfaceInfo[]={((QtWindowInterface)surface).isOpenGl(), surface.getId(), surface.getLeft(), surface.getTop(), surface.getRight(), surface.getBottom()};
	            outState.putIntArray("Surface_"+i, surfaceInfo);
	        }
        }
        outState.putBoolean("FullScreen",fullScreen);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        Log.i(QtApplication.QtTAG, "onRestoreInstanceState");
        super.onRestoreInstanceState(savedInstanceState);
        Log.i(QtApplication.QtTAG, "onRestoreInstanceState");
        QtMainView view = QtApplication.getView();
        if (view != null)
        {
	        int surfaces=savedInstanceState.getInt("Surfaces");
	        for (int i=0;i<surfaces;i++)
	        {
	            int surfaceInfo[]= {0,0,0,0,0,0};
	            surfaceInfo=savedInstanceState.getIntArray("Surface_"+i);
	            if (surfaceInfo[0]==1) // OpenGl Surface
	                view.addView(new QtGlWindow(this, surfaceInfo[1], surfaceInfo[2], surfaceInfo[3], surfaceInfo[4], surfaceInfo[5]),i);
	            else
	                view.addView(new QtWindow(this, surfaceInfo[1], surfaceInfo[2], surfaceInfo[3], surfaceInfo[4], surfaceInfo[5]), i);
	        }
        }
        fullScreen=savedInstanceState.getBoolean("FullScreen");
        if (fullScreen)
        	enterFullScreen();
        synchronized (QtApplication.getActivityMutex())
        {
        	Iterator<Runnable> itr=QtApplication.getLostActions().iterator();
        	while(itr.hasNext())
        		runOnUiThread(itr.next());
        	QtApplication.clearLostActions();
		}
    }
    
    void processEvents(long miliseconds)
    {
    	mHandler.sendMessageDelayed(mHandler.obtainMessage(ProcessEvents), miliseconds);
    }
}
