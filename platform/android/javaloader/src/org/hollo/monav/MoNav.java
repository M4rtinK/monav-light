package org.hollo.monav;

//import android.app.Activity;
import android.os.Bundle;
import com.nokia.qt.android.QtActivity;
import android.location.LocationListener;
import android.location.Location;
import android.location.LocationManager;
import android.app.AlertDialog;
import android.content.DialogInterface;

public class MoNav extends QtActivity implements LocationListener
{
    private LocationManager lm;

    public MoNav()
    {
        setApplication("monav");
    }
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        initLocation();
    }
    private void initLocation()
    {
        lm= (LocationManager) this.getSystemService(LOCATION_SERVICE);
        lm.requestLocationUpdates(LocationManager.GPS_PROVIDER, 1000, 10, this);
    }
    public void onLocationChanged(Location loc) {
        if (loc != null) {
            positionUpdate(loc.getLatitude(), loc.getLongitude(),
                           loc.getAltitude(), loc.getBearing(), loc.getSpeed());
        }
    }

    public void onProviderDisabled(String provider) {
        // TODO Auto-generated method stub
    }

    public void onProviderEnabled(String provider) {
        // TODO Auto-generated method stub
    }

    public void onStatusChanged(String provider, int status,
        Bundle extras) {
        // TODO Auto-generated method stub
    }

}

