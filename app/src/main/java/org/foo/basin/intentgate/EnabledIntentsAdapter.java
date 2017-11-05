package org.foo.basin.intentgate;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Switch;

import java.util.Map;

/**
 * Created by il on 05.11.2017.
 */

public class EnabledIntentsAdapter extends ArrayAdapter<Map.Entry<String, Boolean>> implements View.OnClickListener {

    private LayoutInflater inflater;

    public EnabledIntentsAdapter(@NonNull Context context, int resource) {
        super(context, resource);
        inflater = LayoutInflater.from(context);
    }

    @NonNull
    @Override
    public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent) {
        ViewHolder viewHolder;
        if (convertView == null) {
            convertView = inflater.inflate(R.layout.simple_list_item_1, null);
            viewHolder = new ViewHolder();
            viewHolder.sw = (Switch) convertView.findViewById(R.id.switch1);
            viewHolder.sw.setTag(viewHolder);
            viewHolder.sw.setOnClickListener(this);
            convertView.setTag(viewHolder);
        } else {
            viewHolder = (ViewHolder) convertView.getTag();
        }

        Map.Entry<String, Boolean> item = getItem(position);
        viewHolder.sw.setText(item.getKey());
        viewHolder.sw.setChecked(Boolean.TRUE.equals(item.getValue()));
        viewHolder.position = position;
        return convertView;
    }

    static class ViewHolder {
        Switch sw;
        int position;
    }

    @Override
    public void onClick(View v) {
        ViewHolder viewHolder = (ViewHolder)v.getTag();
        getItem(viewHolder.position).setValue(viewHolder.sw.isChecked());
    }
}
