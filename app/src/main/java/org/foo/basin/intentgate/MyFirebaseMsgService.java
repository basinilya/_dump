package org.foo.basin.intentgate;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.util.Base64;
import android.util.JsonReader;
import android.util.JsonToken;
import android.util.JsonWriter;
import android.util.Log;

import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;

import java.io.IOException;
import java.io.StringReader;
import java.io.StringWriter;
import java.io.UnsupportedEncodingException;
import java.util.Map;
import java.util.TreeMap;

public class MyFirebaseMsgService extends FirebaseMessagingService {

    private static final String TAG = "MyFirebaseMsgService";

    public static final String KEY_WHOLE = "whole";

    private static final String KEY_ACTION = "action";

    private static final String KEY_DATA = "data";

    private static final String KEY_EXTRA = "extra";

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

        // block broadcast messages
        String from = remoteMessage.getFrom();
        if (from == null || from.startsWith("/")) {
            Log.d(TAG, "blocked");
            return;
        }

        // Check if message contains a data payload.
        if (remoteMessage.getData().size() > 0) {
            Map<String, String> allProps = remoteMessage.getData();
            Log.d(TAG, "Message data payload: " + allProps);

            Intent intent = new Intent();

            TreeMap<String, String> keyProps = new TreeMap<>(remoteMessage.getData());
            keyProps.remove(KEY_EXTRA);

            String sUri = allProps.get(KEY_DATA);
            if (sUri != null) {
                Uri uri = Uri.parse(sUri);
                intent.setData(uri);

                String scheme = uri.getScheme();
                keyProps.put(KEY_DATA, scheme + ":");
            }

            StringWriter wr = new StringWriter();
            try {
                JsonWriter jsonw = new JsonWriter(wr);
                jsonw.setIndent("  ");
                jsonw.beginObject();
                for (Map.Entry<String, String> keyProp : keyProps.entrySet()) {
                    jsonw.name(keyProp.getKey()).value(keyProp.getValue());
                }
                jsonw.endObject();
                jsonw.close();
            } catch (IOException e) {
                //
            }
            String ser = wr.toString();
            String key = encode(ser);

            if (prefs.getBoolean(key, false)) {
                Log.d(TAG, "enabled");
                Object o = Intent.ACTION_VIEW;

                int flags = Intent.FLAG_ACTIVITY_NEW_TASK;
                intent.setFlags(flags);

                intent.setAction(allProps.get(KEY_ACTION));
                String sExtra = allProps.get(KEY_EXTRA);
                if (sExtra != null) {
                    Bundle extra = new Bundle();
                    JsonReader jsonr = new JsonReader(new StringReader(sExtra));
                    try {
                        jsonr.beginObject();
                        convObj(jsonr, extra);
                        jsonr.endObject();
                    } catch (Exception e) {
                        Log.e(TAG, "", e);
                        return;
                    }
                    intent.putExtras(extra);
                }
                startActivity(intent);

            } else if (!prefs.contains(key)) {
                Log.d(TAG, "unknown");
                Map<String, ?> all = prefs.getAll();

                Intent notifIntent = new Intent(getApplicationContext(), MainActivity.class);
                notifIntent.putExtra(KEY_WHOLE, ser);
                PendingIntent pe = PendingIntent.getActivity(
                        getApplicationContext(), 0, notifIntent, PendingIntent.FLAG_UPDATE_CURRENT);
                Notification notif = new Notification.Builder(this)
                        .setWhen(System.currentTimeMillis())
                        .setContentIntent(pe)
                        .setContentTitle(getResources().getString(R.string.title_unknown_intent) )
                        .setContentText(ser)
                        .setSmallIcon(R.mipmap.ic_launcher)
                        .setLargeIcon(BitmapFactory.decodeResource(getResources(),
                                R.mipmap.ic_launcher))
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

    public static String encode(String title) {
        try {
            String key = new String(Base64.encode(
                    title.getBytes("UTF-8"), Base64.URL_SAFE | Base64.NO_WRAP)
                    , "UTF-8");
            return key;
        } catch (UnsupportedEncodingException e) {
            throw new RuntimeException(e);
        }
    }

    public static String decode(String key) {
        try {
            String title = new String(Base64.decode(
                    key.getBytes("UTF-8"), Base64.URL_SAFE), "UTF-8");
            return title;
        } catch (UnsupportedEncodingException e) {
            throw new RuntimeException(e);
        }
    }

    private static void convObj(JsonReader jsonr, Bundle bundle) throws IOException {
        while(jsonr.hasNext()) {
            convField(jsonr, bundle);
        }
    }

    private static void convField(JsonReader jsonr, Bundle bundle) throws IOException {
        String key = jsonr.nextName();
        JsonToken poke = jsonr.peek();
        switch (poke) {
            case BEGIN_OBJECT:
                jsonr.beginObject();

                Bundle bundleVal = new Bundle();

                convObj(jsonr, bundleVal);

                bundle.putBundle(key, bundleVal);
                jsonr.endObject();
                break;
            case STRING:
                String sVal = jsonr.nextString();
                bundle.putString(key, sVal);
                break;
            case NUMBER:
                double dVal = jsonr.nextDouble();
                bundle.putDouble(key, dVal);
                break;
            case BOOLEAN:
                boolean bVal = jsonr.nextBoolean();
                bundle.putBoolean(key, bVal);
                break;
            case NULL:
                bundle.putString(key, jsonr.nextString());
                break;
            default:
                throw new IllegalStateException("unexpected " + poke);
        }
    }


    public static String getKnownIntentsPreferencesName(Context context) {
        return context.getPackageName() + "_known_intents";
    }

}
