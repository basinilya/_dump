package org.foo.basin.smssender;

import android.Manifest;
import android.app.Activity;
import android.content.ContentValues;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.telephony.SmsManager;
import android.util.Log;
import android.widget.Toast;

public class SenderActivity extends Activity {

    private static final String TAG = "SenderActivity";

    private static final String SMSTO = "smsto";
    private static final String SMS_BODY = "sms_body";
    private static final String USER_INFO = "user_info";

    private static final int PERMISSION_REQUEST_CODE = 1;

    private String getUserInfo() {
        return "dummy:dummy";
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            if (checkSelfPermission(Manifest.permission.SEND_SMS)
                    == PackageManager.PERMISSION_DENIED) {
                Log.d(TAG, "permission denied to SEND_SMS - requesting it");
                String[] permissions = {Manifest.permission.SEND_SMS};
                requestPermissions(permissions, PERMISSION_REQUEST_CODE);
                return;
            }
        }
        sendIt();
        finish();
    }

    private void sendIt() {

        String phoneNumber, message;

        Intent intent = getIntent();
        Uri uri = intent.getData();
        if (uri == null || !SMSTO.equals(uri.getScheme())) return;
        if (!getUserInfo().equals(intent.getStringExtra(USER_INFO))) {
            Log.d(TAG, "bad password");
            return;
        }
        message = intent.getStringExtra(SMS_BODY);
        phoneNumber = uri.getSchemeSpecificPart();
        if (message == null || phoneNumber == null) return;

        int i = phoneNumber.indexOf('?');
        if (i != -1) {
            phoneNumber = phoneNumber.substring(0, i);
        }

        SmsManager smsMgr = SmsManager.getDefault();
        try {
            smsMgr.sendTextMessage(phoneNumber, null, message, null, null);

            ContentValues values = new ContentValues();
            values.put("address", phoneNumber);
            values.put("body", message);
            getContentResolver().insert(Uri.parse("content://sms/sent"), values);
        } catch (Exception e) {
            Log.e(TAG, "", e);
            Toast.makeText(this, e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
            return;
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        sendIt();
        finish();
    }
}
