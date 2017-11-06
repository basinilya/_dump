package org.foo.basin.intentgate;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListAdapter;
import android.widget.ListView;

import java.util.Date;
import java.util.Map;

public class MainActivity extends PreferenceActivity implements AdapterView.OnItemLongClickListener {

    private static final String TAG = "MainActivity";
    private PreferenceCategory targetCategory;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        PreferenceManager prefMgr = getPreferenceManager();
        prefMgr.setSharedPreferencesName(getKnownIntentsPreferencesName());
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

    public String getKnownIntentsPreferencesName() {
        return getPackageName() + "_known_intents";
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
