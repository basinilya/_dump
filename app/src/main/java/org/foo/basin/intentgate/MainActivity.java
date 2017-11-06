package org.foo.basin.intentgate;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v4.view.MenuItemCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.view.SupportMenuInflater;
import android.support.v7.widget.ShareActionProvider;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.Toast;

import com.google.firebase.iid.FirebaseInstanceId;
import com.google.firebase.messaging.FirebaseMessaging;

import java.util.Date;
import java.util.Map;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    private ShareActionProvider mShareActionProvider;

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.menu_item_unreg:
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
                            String s = e == null ? "unregistered" : e.getLocalizedMessage();
                            Toast.makeText(MainActivity.this, s, Toast.LENGTH_LONG).show();
                            if (e == null) {
                                onTokenRefresh();
                            }
                        }
                    }.execute();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_main, menu);
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

    public static final String ON_TOKEN_REFRESH = "onTokenRefresh";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        LocalBroadcastManager.getInstance(this).registerReceiver(
                mMessageReceiver, new IntentFilter(ON_TOKEN_REFRESH));

        FirebaseMessaging.getInstance().subscribeToTopic("news");

        getFragmentManager().beginTransaction()
                .replace(android.R.id.content, new KnownIntentsFragment()).commit();

    }

    @Override
    protected void onDestroy() {
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mMessageReceiver);
        super.onDestroy();
    }

}
