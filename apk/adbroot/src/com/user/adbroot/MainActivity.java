package com.user.adbroot;

import android.app.Activity;
import android.os.Bundle;

public class MainActivity extends Activity {
    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        // при первом запуске сразу применяем
        try {
            Runtime.getRuntime().exec(new String[]{"/system/bin/setprop","service.adb.root","1"}).waitFor();
            Runtime.getRuntime().exec(new String[]{"/system/bin/setprop","ctl.restart","adbd"}).waitFor();
        } catch (Throwable ignored) { }
        finish();
    }
}
