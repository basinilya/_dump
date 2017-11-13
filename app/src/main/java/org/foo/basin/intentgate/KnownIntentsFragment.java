package org.foo.basin.intentgate;

import android.content.Context;
import android.content.Intent;
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

import java.util.Map;
import java.util.TreeMap;

public class KnownIntentsFragment extends PreferenceFragment implements AdapterView.OnItemLongClickListener {

    private static final String TAG = "KnownIntentsFragment";
    private PreferenceCategory targetCategory;
    private int scrollTo;
    private ListView currentListView;

    @Override
    public void onStart() {
        super.onStart();

        Intent intent = getActivity().getIntent();
        String ser = null;
        if (intent != null) {
            ser = intent.getStringExtra(MyFirebaseMsgService.KEY_WHOLE);
        }

        PreferenceManager prefMgr = getPreferenceManager();

        SharedPreferences prefs = prefMgr.getSharedPreferences();
        String key;
        if (ser != null) {
            key = MyFirebaseMsgService.encode(ser);
            if (!prefs.contains(key)) {
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(key, false);
                editor.apply();
                targetCategory.removeAll();
            }
        }

        if (targetCategory.getPreferenceCount() == 0) {
            TreeMap<String, String> knownIntents = new TreeMap<>();
            for (String key2 : prefs.getAll().keySet()) {
                String title = MyFirebaseMsgService.decode(key2);
                knownIntents.put(title, key2);
            }

            scrollTo = -1;
            int i = 0;
            for (Map.Entry<String, String> x : knownIntents.entrySet()) {
                CheckBoxPreference checkBoxPreference = new CheckBoxPreference(getActivity());
                checkBoxPreference.setLayoutResource(R.layout.preference_wrapping);
                String title = x.getKey();
                key = x.getValue();
                if (title.equals(ser)) {
                    checkBoxPreference.setSummary(R.string.pref_summary_new);
                    scrollTo = i;
                }
                checkBoxPreference.setTitle(title);
                checkBoxPreference.setKey(key);
                targetCategory.addPreference(checkBoxPreference);
                i++;
            }
        }

        if (currentListView != null && scrollTo != -1) {
            currentListView.smoothScrollToPosition(scrollTo);
            scrollTo = -1;
        }
    }

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        PreferenceManager prefMgr = getPreferenceManager();
        prefMgr.setSharedPreferencesName(MyFirebaseMsgService.getKnownIntentsPreferencesName(getActivity()));
        prefMgr.setSharedPreferencesMode(Context.MODE_PRIVATE);
        addPreferencesFromResource(R.xml.prefs_known_intents);

        PreferenceScreen screen = getPreferenceScreen();
        int count = screen.getPreferenceCount();
        Log.d(TAG, "preference count: " + count);


        targetCategory = new CustomPreferenceCategory(getActivity());
        targetCategory.setTitle(R.string.prefs_known_intents);
        screen.addPreference(targetCategory);
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
            currentListView = (ListView)parent;
            currentListView.setOnItemLongClickListener(KnownIntentsFragment.this);
            View res = super.onCreateView(parent);;
            if (scrollTo != -1) {
                currentListView.smoothScrollToPosition(scrollTo);
                scrollTo = -1;
            }
            return res;
        }
    }

}
