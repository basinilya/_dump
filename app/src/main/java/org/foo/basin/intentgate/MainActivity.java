package org.foo.basin.intentgate;

import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.util.Log;

import java.util.Map;

public class MainActivity extends PreferenceActivity {

    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        PreferenceManager prefMgr = getPreferenceManager();
        prefMgr.setSharedPreferencesName(getKnownIntentsPreferencesName());
        prefMgr.setSharedPreferencesMode(MODE_PRIVATE);
        addPreferencesFromResource(R.xml.preferences);
        PreferenceScreen screen = getPreferenceScreen();
        int count = screen.getPreferenceCount();
        Log.d(TAG, "preference count: " + count);
        for (int i = 0 ; i < count; i++) {
            //
            Preference preference = screen.getPreference(i);
            Log.d(TAG, "preference class: " + preference.getClass().getSimpleName());
            Log.d(TAG, "preference key: " + preference.getKey() );
        }
        PreferenceCategory targetCategory = (PreferenceCategory)findPreference("known_intents");

        Map<String, ?> knownIntents = prefMgr.getSharedPreferences().getAll();
        for (Map.Entry<String, ?> x : knownIntents.entrySet()) {
            CheckBoxPreference checkBoxPreference = new CheckBoxPreference(this);
            String key = x.getKey();
            checkBoxPreference.setKey(key);
            checkBoxPreference.setTitle(key);
            targetCategory.addPreference(checkBoxPreference);
        }

/*
*/
        //Map<String, ?> knownIntents = PreferenceManager.getDefaultSharedPreferences(this).getAll();
        //getSharedPreferences(getKnownIntentsPreferencesName(), MODE_PRIVATE);
/*
        for (Map.Entry<String, Boolean> x : knownIntents.entrySet()) {
            adapter.add(x);
        }
        */
    }

    public String getKnownIntentsPreferencesName() {
        return getPackageName() + "_knownIntents";
    }

}
