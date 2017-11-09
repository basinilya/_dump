package org.foo.basin.intentgate;

import static android.util.JsonToken.*;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.util.JsonReader;
import android.util.JsonToken;
import android.util.JsonWriter;
import android.util.Log;

import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;

import java.io.IOException;
import java.io.StringReader;
import java.io.StringWriter;
import java.util.ArrayList;
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

        // Check if message contains a data payload.
        if (remoteMessage.getData().size() > 0) {
            Map<String, String> allProps = remoteMessage.getData();
            Log.d(TAG, "Message data payload: " + allProps);
            TreeMap<String, String> keyProps = new TreeMap<>(remoteMessage.getData());
            keyProps.remove(KEY_DATA);
            keyProps.remove(KEY_EXTRA);
            StringWriter wr = new StringWriter();
            try {
                JsonWriter jsonw = new JsonWriter(wr);
                jsonw.setIndent("  ");
                jsonw.beginObject();
                for (Map.Entry<String, String> keyProp : keyProps.entrySet()) {
                    jsonw.name(keyProp.getKey()).value(keyProp.getValue());
                }
                jsonw.close();
            } catch (IOException e) {
                //
            }
            String ser = wr.toString();

            if (prefs.getBoolean(ser, false)) {
                Log.d(TAG, "enabled");

                Intent intent = new Intent();
                intent.setAction(allProps.get(KEY_ACTION));
                String sUri = allProps.get(KEY_DATA);
                if (sUri != null) {
                    intent.setData(Uri.parse(sUri));
                }
                String sExtra = allProps.get(KEY_EXTRA);
                if (sExtra != null) {
                    Bundle extra = intent.getExtras();
                    JsonReader jsonr = new JsonReader(new StringReader(sExtra));
                    try {
                        jsonr.beginObject();
                        aaa(jsonr, extra);
                        jsonr.endObject();
                    } catch (IOException e) {
                        Log.e(TAG, "", e);
                    }
                }


            } else if (!prefs.contains(ser)) {
                Log.d(TAG, "unknown");

                Intent intent = new Intent(getApplicationContext(), MainActivity.class);
                intent.getExtras().putString(KEY_WHOLE, ser);
                PendingIntent pe = PendingIntent.getActivity(
                        getApplicationContext(), 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
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

    private static void aaa(JsonReader jsonr, Bundle extra) throws IOException {
        while(jsonr.hasNext()) {
            String key = jsonr.nextName();
            JsonToken poke = jsonr.peek();
            switch (poke) {
                case BEGIN_ARRAY:
                    jsonr.beginArray();
                    ArrayList<Object> list = new ArrayList<Object>();
                    jsonr.endArray();
                    break;
                case BEGIN_OBJECT: break;
                case STRING:
                    String sVal = jsonr.nextString();
                    extra.putString(key, sVal);
                    break;
                case NUMBER:
                    double dVal = jsonr.nextDouble();
                    extra.putDouble(key, dVal);
                    break;
                case BOOLEAN:
                    boolean bVal = jsonr.nextBoolean();
                    extra.putBoolean(key, bVal);
                    break;
                case NULL: break;
                default:
                    throw new IllegalStateException("unexpected " + poke);
            }

        }
    }

    public static String getKnownIntentsPreferencesName(Context context) {
        return context.getPackageName() + "_known_intents";
    }

}
