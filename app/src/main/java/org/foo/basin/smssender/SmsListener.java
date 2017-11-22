package org.foo.basin.smssender;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Telephony;
import android.telephony.SmsMessage;
import android.util.Log;

/**
 * Created by basin on 22.11.2017.
 */

// @TargetApi(Build.VERSION_CODES.KITKAT)
public class SmsListener extends BroadcastReceiver {

    private static final String TAG = "SmsListener";

    private static final String SMS_RECEIVED_ACTION = Telephony.Sms.Intents.SMS_RECEIVED_ACTION;

    @Override
    public void onReceive(Context context, Intent intent) {
        if(SMS_RECEIVED_ACTION.equals(intent.getAction())) {
            Bundle bundle = intent.getExtras();           //---get the SMS message passed in---
            SmsMessage[] msgs = null;
            String msg_from;
            if (bundle != null){
                //---retrieve the SMS message received---
                try{
                    Object[] pdus = (Object[]) bundle.get("pdus");
                    msgs = new SmsMessage[pdus.length];
                    StringBuilder fullMsg = new StringBuilder();
                    for(int i=0; i<msgs.length; i++){
                        msgs[i] = SmsMessage.createFromPdu((byte[])pdus[i]);
                        msg_from = msgs[i].getOriginatingAddress();
                        fullMsg.append(msgs[i].getMessageBody());
                    }
                    String contactNumber = msgs[0].getOriginatingAddress();
                    Log.d(TAG, "onReceive: " + contactNumber + ": " + fullMsg.toString());
                }catch(Exception e){
//                            Log.d("Exception caught",e.getMessage());
                }
            }
            // Toast.makeText(context, "received", Toast.LENGTH_LONG).show();
            // ((Object)null).toString();
        }
    }
}
