package org.foo.basin.intentgate;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.util.Log;

import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;

import java.util.Map;

public class MyFirebaseMsgService extends FirebaseMessagingService {

    private static final String TAG = "MyFirebaseMsgService";

    private static final String KEY_INTENT = "intent";

    private SharedPreferences prefs;

    private NotificationManager nm;

    @Override
    public void onCreate() {
        super.onCreate();
        prefs = getSharedPreferences(getKnownIntentsPreferencesName(this), MODE_PRIVATE);
        nm = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
    }

    @Override
    public void onMessageReceived(RemoteMessage remoteMessage) {

        Log.d(TAG, "From: " + remoteMessage.getFrom());

        // Check if message contains a data payload.
        if (remoteMessage.getData().size() > 0) {
            Log.d(TAG, "Message data payload: " + remoteMessage.getData());
            Map<String, String> data = remoteMessage.getData();
            String intent = data.get(KEY_INTENT);

            if (prefs.getBoolean(intent, false)) {
                Log.d(TAG, "enabled");
            } else if (!prefs.contains(intent)) {
                Log.d(TAG, "unknown");

                PendingIntent pe = PendingIntent.getActivity(
                        getApplicationContext(), 0, new Intent(), PendingIntent.FLAG_UPDATE_CURRENT);
                Notification notif = new Notification.Builder(this)
                        .setWhen(System.currentTimeMillis())
                        .setContentIntent(pe)
                        .setContentTitle("dfsfsdfsdf")
                        .setContentText("sdfsdfsdf")
                        .setSmallIcon(R.mipmap.ic_launcher)
                        .setAutoCancel(true)
                        .getNotification();
                nm.notify(1, notif);
            }
        }

        // Check if message contains a notification payload.
        if (remoteMessage.getNotification() != null) {
            Log.d(TAG, "Message Notification Body: " + remoteMessage.getNotification().getBody());
        }
    }


    public static String getKnownIntentsPreferencesName(Context context) {
        return context.getPackageName() + "_known_intents";
    }

}
