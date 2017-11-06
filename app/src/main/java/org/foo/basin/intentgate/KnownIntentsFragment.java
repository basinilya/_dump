package org.foo.basin.intentgate;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListAdapter;
import android.widget.ListView;

import java.util.Date;
import java.util.Map;
import java.util.TreeMap;

public class KnownIntentsFragment extends PreferenceFragment implements AdapterView.OnItemLongClickListener {

    private static final String TAG = "KnownIntentsFragment";
    private PreferenceCategory targetCategory;

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);


        PreferenceManager prefMgr = getPreferenceManager();
        prefMgr.setSharedPreferencesName(getKnownIntentsPreferencesName(getActivity()));
        prefMgr.setSharedPreferencesMode(Context.MODE_PRIVATE);
        addPreferencesFromResource(R.xml.prefs_known_intents);

        PreferenceScreen screen = getPreferenceScreen();
        int count = screen.getPreferenceCount();
        Log.d(TAG, "preference count: " + count);


        targetCategory = new CustomPreferenceCategory(getActivity());
        targetCategory.setTitle(R.string.prefs_known_intents);
        screen.addPreference(targetCategory);

        SharedPreferences.Editor editor = prefMgr.getSharedPreferences().edit();
        editor.putBoolean(new Date().toString(), false);
        editor.apply();

        TreeMap<String, Object> knownIntents = new TreeMap<>(prefMgr.getSharedPreferences().getAll());
        for (Map.Entry<String, ?> x : knownIntents.entrySet()) {
            CheckBoxPreference checkBoxPreference = new CheckBoxPreference(getActivity());
            String key = x.getKey();
            checkBoxPreference.setKey(key);
            checkBoxPreference.setTitle(key);
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

    public class CustomPreferenceCategory extends PreferenceCategory {
        public CustomPreferenceCategory(Context context) {
            super(context);
        }

        public CustomPreferenceCategory(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        @Override
        protected View onCreateView(ViewGroup parent) {
            ListView listView = (ListView)parent;
            listView.setOnItemLongClickListener(KnownIntentsFragment.this);
            return super.onCreateView(parent);
        }
    }

}
