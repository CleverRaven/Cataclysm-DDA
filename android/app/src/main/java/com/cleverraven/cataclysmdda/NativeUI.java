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

    private class SingleChoiceList {
        private Semaphore semaphore = new Semaphore(0, true);

        private int singleChoiceResponse;

        private final int cancel = -1;
        private final int init = -2;

        private String[] mixOptionsText(final String[] options, final boolean[] enabled) {
            String[] result = new String[options.length];
            for (int i = 0; i < options.length; i++) {
                if (enabled[i]) {
                    result[i] = options[i];
                } else {
                    result[i] = String.format("%s [%s]", options[i], activity.getString(R.string.unavailable));
                }
            }
            return result;
        }

        public int singleChoiceList(final String text, final String[] options, final boolean[] enabled) {
            singleChoiceResponse = init;

            final String[] choices = mixOptionsText(options, enabled);

            while (singleChoiceResponse == init
                    || (singleChoiceResponse != cancel && enabled[singleChoiceResponse] == false)) {
                activity.runOnUiThread(new Runnable() {
                    public void run() {
                        AlertDialog dialog = new AlertDialog.Builder(activity, R.style.AlertDialogTheme)
                                .setTitle(text)
                                .setItems(choices, new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog, int which) {
                                        singleChoiceResponse = which;
                                        semaphore.release();
                                    }
                                })
                                .setOnCancelListener(new DialogInterface.OnCancelListener() {
                                    public void onCancel(DialogInterface dialog) {
                                        singleChoiceResponse = cancel;
                                        semaphore.release();
                                    }
                                })
                                .create();
                        dialog.show();
                    }
                });

                try {
                    semaphore.acquire();
                } catch (InterruptedException ex) {
                    // No-op
                }

                if (singleChoiceResponse != cancel && enabled[singleChoiceResponse] == false) {
                    new Popup().popup(activity.getString(R.string.unavailableOption));
                }
            }

            return singleChoiceResponse;
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

    public int singleChoiceList(final String text, final String[] options, final boolean[] enabled) {
        final SingleChoiceList singleChoiceList = new SingleChoiceList();
        return singleChoiceList.singleChoiceList(text, options, enabled);
    }
}
