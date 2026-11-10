package com.user.adbroot;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/**
 * Устройство стартует с adbd в uid=2000(shell). persistent-настройка u-boot
 * (androidboot.rabbith.adb_root=1 + androidboot.selinux=permissive) разрешает
 * `adb root`, но его надо инициировать. Делаем это локально:
 *   setprop service.adb.root 1  +  setprop ctl.restart adbd
 * -> adbd перезапускается в u:r:su:s0 / uid=0.
 */
public class BootReceiver extends BroadcastReceiver {
    static class Worker implements Runnable {
        private final PendingResult pr;
        Worker(PendingResult p) { this.pr = p; }
        private static void sh(String k, String v) {
            try {
                Runtime.getRuntime().exec(new String[]{"/system/bin/setprop", k, v}).waitFor();
            } catch (Throwable ignored) { }
        }
        @Override public void run() {
            sh("service.adb.root", "1");
            sh("ctl.restart", "adbd");
            try { Thread.sleep(8000); } catch (InterruptedException ignored) { }
            sh("ctl.restart", "adbd");   // страховка, если adbd ещё не был готов
            pr.finish();
        }
    }
    @Override public void onReceive(Context ctx, Intent intent) {
        PendingResult pr = goAsync();
        new Thread(new Worker(pr)).start();
    }
}
