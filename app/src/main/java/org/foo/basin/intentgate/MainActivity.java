package org.foo.basin.intentgate;

import android.app.AlertDialog;
import android.app.NotificationManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.AsyncTask;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v4.view.MenuItemCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.ShareActionProvider;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

import com.google.firebase.iid.FirebaseInstanceId;

import java.util.List;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    public static final String ON_TOKEN_REFRESH = "onTokenRefresh";

    private static final String PROTECTED_APP = "protected";

    private static final String MANUF_HUAWEI = "huawei";

    private ShareActionProvider mShareActionProvider;

    private SharedPreferences sp;

    private boolean alreadyProtected;

    private void protectApp() {
        AlertDialog.Builder builder  = new AlertDialog.Builder(this);
        builder.setTitle(R.string.protect_app)
                // .setMessage(R.string.protectApp)
                .setMultiChoiceItems(R.array.dont_show_again, new boolean[]{alreadyProtected}, new DialogInterface.OnMultiChoiceClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which, boolean isChecked) {
                        alreadyProtected = isChecked;
                    }
                })
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        Intent intent = mkProtectAppIntent();
                        try {
                            startActivity(intent);
                        } catch (Exception e) {
                            Log.e(TAG, "", e);
                            Toast.makeText(MainActivity.this, e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
                            return;
                        }
                        saveProtectChoice();
                    }
                })
                .setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        saveProtectChoice();
                    }
                })
                .create().show();
    }

    private void saveProtectChoice() {
        sp.edit().putBoolean(PROTECTED_APP, alreadyProtected).apply();
    }

    private static Intent mkProtectAppIntent() {
        return new Intent()
                .setComponent(new ComponentName("com.huawei.systemmanager", "com.huawei.systemmanager.optimize.process.ProtectActivity"));

    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.menu_item_protect_app:
                protectApp();
                return true;
            case R.id.menu_item_unreg:
                unreg();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    private void unreg() {
        new AsyncTask<Void, Void, Exception>() {
            @Override
            protected Exception doInBackground(Void... voids) {
                try {
                    FirebaseInstanceId.getInstance().deleteInstanceId();
                } catch (Exception e) {
                    Log.e(TAG, "", e);
                    return e;
                }
                return null;
            }

            @Override
            protected void onPostExecute(Exception e) {
                String s = e == null ? getResources().getString(R.string.unregistered) : e.getLocalizedMessage();
                Toast.makeText(MainActivity.this, s, Toast.LENGTH_LONG).show();
                if (e == null) {
                    onTokenRefresh();
                }
            }
        }.execute();
    }

    private boolean isProtectableApp() {

        Intent intent = mkProtectAppIntent();

        PackageManager pm = getPackageManager();
        List<ResolveInfo> infos = pm.queryIntentActivities(intent, 0);
        // return infos != null && infos.size() != 0);

        return MANUF_HUAWEI.equalsIgnoreCase(android.os.Build.MANUFACTURER)
                || "unknown".equalsIgnoreCase(android.os.Build.MANUFACTURER);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_main, menu);

        if(isProtectableApp()) {
            menu.findItem(R.id.menu_item_protect_app).setVisible(true);
            alreadyProtected = sp.getBoolean(PROTECTED_APP, false);
            if (!alreadyProtected) {
                alreadyProtected = true;
                protectApp();
            }
        }

        MenuItem shareItem = menu.findItem(R.id.menu_item_share);
        mShareActionProvider = (ShareActionProvider)MenuItemCompat.getActionProvider(shareItem);

        onTokenRefresh();
        return true;
    }

    private BroadcastReceiver mMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            onTokenRefresh();
        }
    };

    public void onTokenRefresh() {
        if (mShareActionProvider != null) {
            String refreshedToken = FirebaseInstanceId.getInstance().getToken();

            Log.d(TAG, "Refreshed token: " + refreshedToken);
            if (refreshedToken != null) {
                Toast.makeText(this, "" + refreshedToken, Toast.LENGTH_LONG).show();
            }

            Intent sendIntent = new Intent();
            sendIntent.setAction(Intent.ACTION_SEND);
            sendIntent.putExtra(Intent.EXTRA_TEXT, refreshedToken);
            sendIntent.setType("text/plain");

            mShareActionProvider.setShareIntent(sendIntent);
        }
    }

    private NotificationManager nm;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        sp = PreferenceManager.getDefaultSharedPreferences(this);
        nm = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

        LocalBroadcastManager.getInstance(this).registerReceiver(
                mMessageReceiver, new IntentFilter(ON_TOKEN_REFRESH));

        getFragmentManager().beginTransaction()
                .replace(android.R.id.content, new KnownIntentsFragment()).commit();

    }

    @Override
    protected void onDestroy() {
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mMessageReceiver);
        super.onDestroy();
    }
}
