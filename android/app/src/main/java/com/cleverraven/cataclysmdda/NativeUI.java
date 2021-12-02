package com.cleverraven.cataclysmdda;

import java.util.concurrent.Semaphore;

import android.app.AlertDialog;
import android.content.DialogInterface;

public class NativeUI {
    enum YesNoDialogResponse {
        YES,
        NO
    }

    private CataclysmDDA activity;

    NativeUI(CataclysmDDA activity) {
        this.activity = activity;
    }

    private class Popup {
        private Semaphore semaphore = new Semaphore(0, true);

        public void popup(final String message) {
            activity.runOnUiThread(new Runnable() {
                public void run() {
                    AlertDialog dialog = new AlertDialog.Builder(activity, R.style.AlertDialogTheme)
                            .setTitle("")
                            .setCancelable(false)
                            .setMessage(message)
                            .setNeutralButton(R.string.ok, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    semaphore.release();
                                }
                            }).create();
                    dialog.show();
                }
            });

            try {
                semaphore.acquire();
            } catch (InterruptedException ex) {
                // No-op
            }
        }
    }

    private class QueryYN {
        private Semaphore semaphore = new Semaphore(0, true);

        private YesNoDialogResponse response;

        public boolean queryYN(final String message) {
            activity.runOnUiThread(new Runnable() {
                public void run() {
                    AlertDialog dialog = new AlertDialog.Builder(activity, R.style.AlertDialogTheme)
                            .setTitle("")
                            .setCancelable(false)
                            .setMessage(message)
                            .setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    response = YesNoDialogResponse.YES;
                                    semaphore.release();
                                }
                            })
                            .setNegativeButton(R.string.no, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    response = YesNoDialogResponse.NO;
                                    semaphore.release();
                                }
                            }).create();
                    dialog.show();
                }
            });

            try {
                semaphore.acquire();
            } catch (InterruptedException ex) {
                // No-op
            }

            return response == YesNoDialogResponse.YES;
        }
    }

    public void popup(final String message) {
        final Popup popup = new Popup();
        popup.popup(message);
    }

    public boolean queryYN(final String message) {
        final QueryYN queryYN = new QueryYN();
        return queryYN.queryYN(message);
    }
}
