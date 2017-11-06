package org.foo.basin.intentgate;

import android.content.Context;
import android.content.Intent;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.widget.Toast;

import com.google.firebase.iid.FirebaseInstanceId;
import com.google.firebase.iid.FirebaseInstanceIdService;


public class MyFirebaseIIDService extends FirebaseInstanceIdService {

    private static final String TAG = "MyFirebaseIIDService";

    @Override
    public void onTokenRefresh() {
        Intent intent = new Intent(MainActivity.ON_TOKEN_REFRESH);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }
}
