package org.foo.basin.intentgate;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.Toast;

import com.google.firebase.FirebaseApp;
import com.google.firebase.messaging.FirebaseMessaging;

import java.util.Date;
import java.util.Map;

public class MainActivity extends PreferenceActivity implements AdapterView.OnItemLongClickListener {

    private static final String TAG = "MainActivity";
    private PreferenceCategory targetCategory;

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Default FirebaseApp is not initialized in this process org.foo.basin.intentgate. Make sure to call FirebaseApp.initializeApp(Context) first.
        //        at android.app.ActivityThread.performLaunchActivity(ActivityThread.java:2325)
        //FirebaseApp.initializeApp(this);

        String refreshedToken = MyFirebaseIIDService.logToken(this);
        if (refreshedToken == null) {
            FirebaseMessaging.getInstance().subscribeToTopic("news");
        }

        PreferenceManager prefMgr = getPreferenceManager();
        prefMgr.setSharedPreferencesName(getKnownIntentsPreferencesName(this));
        prefMgr.setSharedPreferencesMode(MODE_PRIVATE);
        addPreferencesFromResource(R.xml.prefs_known_intents);

        PreferenceScreen screen = getPreferenceScreen();
        int count = screen.getPreferenceCount();
        Log.d(TAG, "preference count: " + count);

        ListView listView = getListView();
        listView.setOnItemLongClickListener(this);

        targetCategory = (PreferenceCategory)findPreference("known_intents");

        SharedPreferences.Editor editor = prefMgr.getSharedPreferences().edit();
        editor.putBoolean(new Date().toString(), false);
        editor.apply();

        Map<String, ?> knownIntents = prefMgr.getSharedPreferences().getAll();
        for (Map.Entry<String, ?> x : knownIntents.entrySet()) {
            CheckBoxPreference checkBoxPreference = new CheckBoxPreference(this);
            String key = x.getKey();
            checkBoxPreference.setKey(key);
            checkBoxPreference.setTitle(key);
            // checkBoxPreference.getView(null, null).setOnLongClickListener();
            targetCategory.addPreference(checkBoxPreference);
        }
    }

    public static String getKnownIntentsPreferencesName(Context context) {
        return context.getPackageName() + "_known_intents";
    }

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
        ListView listView = (ListView) parent;
        ListAdapter listAdapter = listView.getAdapter();
        Object obj = listAdapter.getItem(position);
        if (obj instanceof Preference) {
            Preference pref = (Preference) obj;
            targetCategory.removePreference(pref);
            SharedPreferences.Editor editor = pref.getEditor();
            editor.remove(pref.getKey());
            editor.apply();
            return true;
        }
        return false;
    }
}
