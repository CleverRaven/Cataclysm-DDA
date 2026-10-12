package com.cleverraven.cataclysmdda;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.FileNotFoundException;
import java.io.InputStreamReader;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.DialogInterface;
import android.content.DialogInterface.OnShowListener;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.*;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.ScrollView;
import android.widget.TextView;

import com.cleverraven.cataclysmdda.CataclysmDDA_Helpers;

public class SplashScreen extends Activity {
    private static final String TAG = "Splash";
    private static final int INSTALL_DIALOG_ID = 0;
    private ProgressDialog installDialog;

    private AlertDialog accessibilityServicesAlert;

    public boolean[] mSettingsValues = { false, true, true };
    private int mSystemUiModeIndex = 0;
    private String mScreenOrientation = CataclysmDDA.SCREEN_ORIENTATION_LANDSCAPE;

    private String getVersionName() {
        try {
            Context context = getApplicationContext();
            PackageInfo pInfo = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
            return pInfo.versionName;
        } catch (Exception e) {
            e.printStackTrace();
            return "error";
        }
    }

    private void showCrashAlert() {
        String externalFilesDir = getExternalFilesDir(null).getPath();
        File crashAlertPrompt = new File(externalFilesDir + "/config/crash.log.prompt");
        try {
            crashAlertPrompt.delete();
            if(crashAlertPrompt.exists()) { // Sometimes .delete() doesn't really delete the file and I don't know why
                crashAlertPrompt.getCanonicalFile().delete();
            }
        } catch(IOException e) {
            return;
        }
        File crashLog = new File(externalFilesDir + "/config/crash.log");
        StringBuilder text = new StringBuilder();
        text.append(getString(R.string.crashMessage));
        text.append("\n\n");
        try {
            BufferedReader br = new BufferedReader(new FileReader(crashLog));
            String line;
            while((line = br.readLine()) != null) {
                text.append(line);
                text.append("\n");
            }
            br.close();
        } catch (IOException e) {
            return;
        }
        final String message = text.toString();
        this.runOnUiThread(new Runnable() {
           public void run() {
                AlertDialog errorAlert = new AlertDialog.Builder(SplashScreen.this)
                .setTitle(getString(R.string.crashAlert))
                .setCancelable(false)
                .setMessage(message)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        SplashScreen.this.startGameActivity(false);
                    }
                }).create();
                errorAlert.show();
           }
        });
    }

    @Override
    protected void onStart() {
        Log.e(TAG, "onStart()");
        super.onStart();
    }

    @Override
    protected void onPause() {
        Log.e(TAG, "onPause()");
        super.onPause();
        accessibilityServicesAlert.dismiss();
    }

    @Override
    protected void onResume() {
        Log.e(TAG, "onResume()");
        super.onResume();

        Context context = getApplicationContext();
        String service_names = CataclysmDDA_Helpers.getEnabledAccessibilityServiceNames(context);
        accessibilityServicesAlert.setMessage( String.format( getString(R.string.accessibilityServicesMessage), service_names ) );
        if (!service_names.isEmpty()) {
            accessibilityServicesAlert.show();
        } else {
            SplashScreen.this.installOrRun();
        }
    }
    
    protected void installOrRun() {
        Log.e(TAG, "onCreate()");
        accessibilityServicesAlert.dismiss();
        // Start the game if already installed, otherwise start installing...
        if (getVersionName().equals(PreferenceManager.getDefaultSharedPreferences(getApplicationContext()).getString("installed", ""))) {
            // Show an alert box if the game crashed last time
            String externalFilesDir = getExternalFilesDir(null).getPath();
            File crashAlertPrompt = new File(externalFilesDir + "/config/crash.log.prompt");
            if(crashAlertPrompt.exists()) {
                showCrashAlert();
            } else {
                startGameActivity(false);
            }
        }
        else {
            new InstallProgramTask().execute();
        }
        return;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.e(TAG, "onCreate()");
        super.onCreate(savedInstanceState);
        CataclysmDDA.applyStoredScreenOrientation(this);

        accessibilityServicesAlert = new AlertDialog.Builder(SplashScreen.this)
            .setTitle(getString(R.string.accessibilityServicesTitle))
            .setCancelable(false)
            .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id) {
                    SplashScreen.this.installOrRun();
                    return;
                }
            })
            .setNeutralButton(getString(R.string.showAccessibilitySettings), new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        startActivityForResult(new Intent(android.provider.Settings.ACTION_ACCESSIBILITY_SETTINGS), 0);
                        dialog.dismiss();
                        return;
                    }
            })
            .setNegativeButton(getString(R.string.ignoreFalsePostives), new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        CataclysmDDA_Helpers.saveAccessibilityServiceInfoFalsePositives(getApplicationContext());
                        SplashScreen.this.installOrRun();
                        return;
                    }
            }).create();
    }

    @Override
    public Dialog onCreateDialog(int id) {
        switch (id) {
            case INSTALL_DIALOG_ID:
                installDialog = new ProgressDialog(this);
                installDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
                boolean clean_install = PreferenceManager.getDefaultSharedPreferences(getApplicationContext()).getString("installed", "").isEmpty();
                installDialog.setTitle(getString(clean_install ? R.string.installTitle : R.string.upgradeTitle));
                installDialog.setIndeterminate(true);
                installDialog.setCancelable(false);
                return installDialog;
            default:
                return null;
        }
    }

    private void startGameActivity(boolean delay) {
        if (!delay) {
            runOnUiThread(new StartGameRunnable());
        }
        else {
            // Wait 1.5 seconds, then start game
            Timer timer = new Timer();
            TimerTask gameStartTask = new TimerTask() {
                @Override
                public void run() {
                    runOnUiThread(new StartGameRunnable());
                }
            };
            timer.schedule(gameStartTask, 1500);
        }
    }

    private final class StartGameRunnable implements Runnable {
        @Override
        public void run() {
            Intent intent = new Intent(SplashScreen.this, CataclysmDDA.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
            startActivity(intent);
            finish();
            overridePendingTransition(0, 0);
        }
    }

    private class InstallProgramTask extends AsyncTask<Void, Integer, Boolean> {
        private final List<String> PRESERVE_SUBFOLDERS = Arrays.asList("sound", "mods", "gfx"); // don't delete custom subfolders under these folders
        private final List<String> PRESERVE_FOLDERS = Arrays.asList("font"); // don't delete this folder
        private final List<String> PRESERVE_FILES = Arrays.asList("user-default-mods.json"); // don't delete this file

        private int totalFiles = 0;
        private long countMillis = 0;
        // one copy buffer for all files
        private final byte[] buffer = new byte[256 * 1024];
        // per-file progress updates flood the UI thread
        private static final long PROGRESS_INTERVAL_MILLIS = 100;
        private static final String ASSET_FILE_LIST = "android/asset_files.txt";
        private long lastProgressMillis = 0;
        private long copiedBytes = 0;
        private int installedFiles = 0;

        private AlertDialog installationAlert;
        private AlertDialog settingsAlert;
        private AlertDialog helpAlert;

        @Override
        protected void onPreExecute() {
            installationAlert = new AlertDialog.Builder(SplashScreen.this)
                .setTitle("Installation Failed")
                .setCancelable(false)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        SplashScreen.this.finish();
                        return;
                    }
                }).create();
            showDialog(INSTALL_DIALOG_ID);

            helpAlert = new AlertDialog.Builder(SplashScreen.this)
                .setTitle(getString(R.string.helpTitle))
                .setCancelable(false)
                .setMessage(getString(R.string.helpMessage))
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        settingsAlert.show();
                        return;
                    }
                }).create();

            loadSettingsDefaults();

            settingsAlert = new AlertDialog.Builder(SplashScreen.this)
                .setTitle(getString(R.string.settings))
                .setView(createSettingsView())
                .setCancelable(false)
                .setPositiveButton(getString(R.string.startGame), new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        PreferenceManager.getDefaultSharedPreferences(getApplicationContext()).edit().putBoolean("Software rendering", SplashScreen.this.mSettingsValues[0]).commit();
                        PreferenceManager.getDefaultSharedPreferences(getApplicationContext()).edit().putString(CataclysmDDA.PREF_SYSTEM_UI_MODE, getSelectedSystemUiMode()).commit();
                        PreferenceManager.getDefaultSharedPreferences(getApplicationContext()).edit().putString(CataclysmDDA.PREF_SCREEN_ORIENTATION, SplashScreen.this.mScreenOrientation).commit();
                        PreferenceManager.getDefaultSharedPreferences(getApplicationContext()).edit().putBoolean("Trap Back button", SplashScreen.this.mSettingsValues[1]).commit();
                        PreferenceManager.getDefaultSharedPreferences(getApplicationContext()).edit().putBoolean("Native Android UI", SplashScreen.this.mSettingsValues[2]).commit();
                        SplashScreen.this.startGameActivity(false);
                        return;
                    }
                })
                .setNeutralButton(getString(R.string.showHelp), new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        helpAlert.show();
                        return;
                    }
                }).create();
        }

        private void loadSettingsDefaults() {
            SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
            SplashScreen.this.mSettingsValues[0] = preferences.getBoolean("Software rendering", false);
            SplashScreen.this.mSettingsValues[1] = preferences.getBoolean("Trap Back button", true);
            SplashScreen.this.mSettingsValues[2] = preferences.getBoolean("Native Android UI", true);

            String mode;
            if (preferences.contains(CataclysmDDA.PREF_SYSTEM_UI_MODE)) {
                mode = preferences.getString(
                    CataclysmDDA.PREF_SYSTEM_UI_MODE,
                    CataclysmDDA.SYSTEM_UI_MODE_SYSTEM_BARS);
            } else {
                mode = preferences.getBoolean(CataclysmDDA.PREF_FORCE_FULLSCREEN, false)
                    ? CataclysmDDA.SYSTEM_UI_MODE_EDGE_TO_EDGE
                    : CataclysmDDA.SYSTEM_UI_MODE_SYSTEM_BARS;
            }
            SplashScreen.this.mSystemUiModeIndex = systemUiModeIndex(mode);
            SplashScreen.this.mScreenOrientation = preferences.getString(
                CataclysmDDA.PREF_SCREEN_ORIENTATION, CataclysmDDA.SCREEN_ORIENTATION_LANDSCAPE);
        }

        private ScrollView createSettingsView() {
            ScrollView scrollView = new ScrollView(SplashScreen.this);
            LinearLayout layout = new LinearLayout(SplashScreen.this);
            layout.setOrientation(LinearLayout.VERTICAL);
            int padding = (int)(24 * getResources().getDisplayMetrics().density);
            layout.setPadding(padding, 0, padding, 0);

            TextView displayModeLabel = new TextView(SplashScreen.this);
            displayModeLabel.setText(getString(R.string.androidSystemUiMode));
            layout.addView(displayModeLabel);

            RadioGroup displayModeGroup = new RadioGroup(SplashScreen.this);
            displayModeGroup.setOrientation(RadioGroup.VERTICAL);
            addSystemUiModeButton(displayModeGroup, 0, getString(R.string.androidSystemUiModeSystemBars));
            addSystemUiModeButton(displayModeGroup, 1, getString(R.string.androidSystemUiModeFullscreen));
            addSystemUiModeButton(displayModeGroup, 2, getString(R.string.androidSystemUiModeEdgeToEdge));
            displayModeGroup.check(systemUiModeButtonId(mSystemUiModeIndex));
            displayModeGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(RadioGroup group, int checkedId) {
                    SplashScreen.this.mSystemUiModeIndex = systemUiModeIndexFromButtonId(checkedId);
                }
            });
            layout.addView(displayModeGroup);

            TextView orientationLabel = new TextView(SplashScreen.this);
            orientationLabel.setText(getString(R.string.androidScreenOrientation));
            layout.addView(orientationLabel);

            final String[] orientations = {
                CataclysmDDA.SCREEN_ORIENTATION_LANDSCAPE,
                CataclysmDDA.SCREEN_ORIENTATION_PORTRAIT,
                CataclysmDDA.SCREEN_ORIENTATION_AUTO
            };
            final String[] orientationLabels = {
                getString(R.string.androidScreenOrientationLandscape),
                getString(R.string.androidScreenOrientationPortrait),
                getString(R.string.androidScreenOrientationAuto)
            };
            RadioGroup orientationGroup = new RadioGroup(SplashScreen.this);
            orientationGroup.setOrientation(RadioGroup.VERTICAL);
            for (int i = 0; i < orientations.length; i++) {
                RadioButton button = new RadioButton(SplashScreen.this);
                button.setId(ORIENTATION_BUTTON_ID_BASE + i);
                button.setText(orientationLabels[i]);
                orientationGroup.addView(button, new RadioGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));
                if (orientations[i].equals(mScreenOrientation)) {
                    orientationGroup.check(ORIENTATION_BUTTON_ID_BASE + i);
                }
            }
            orientationGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(RadioGroup group, int checkedId) {
                    SplashScreen.this.mScreenOrientation = orientations[checkedId - ORIENTATION_BUTTON_ID_BASE];
                    SplashScreen.this.setRequestedOrientation(
                        CataclysmDDA.requestedOrientationFor(SplashScreen.this.mScreenOrientation));
                }
            });
            layout.addView(orientationGroup);

            addBooleanSetting(layout, 0, getString(R.string.softwareRendering));
            addBooleanSetting(layout, 1, getString(R.string.trapBackButton));
            addBooleanSetting(layout, 2, getString(R.string.nativeAndroidUI));
            scrollView.addView(layout, new ScrollView.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));
            return scrollView;
        }

        private void addSystemUiModeButton(RadioGroup group, int id, String label) {
            RadioButton button = new RadioButton(SplashScreen.this);
            button.setId(systemUiModeButtonId(id));
            button.setText(label);
            group.addView(button, new RadioGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));
        }

        private static final int ORIENTATION_BUTTON_ID_BASE = 2000;

        private int systemUiModeButtonId(int index) {
            return 1000 + index;
        }

        private int systemUiModeIndexFromButtonId(int buttonId) {
            return buttonId - 1000;
        }

        private void addBooleanSetting(LinearLayout layout, final int index, String label) {
            CheckBox checkBox = new CheckBox(SplashScreen.this);
            checkBox.setText(label);
            checkBox.setChecked(SplashScreen.this.mSettingsValues[index]);
            checkBox.setOnClickListener(v -> SplashScreen.this.mSettingsValues[index] = checkBox.isChecked());
            layout.addView(checkBox, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));
        }

        private String getSelectedSystemUiMode() {
            switch (mSystemUiModeIndex) {
                case 1:
                    return CataclysmDDA.SYSTEM_UI_MODE_FULLSCREEN;
                case 2:
                    return CataclysmDDA.SYSTEM_UI_MODE_EDGE_TO_EDGE;
                default:
                    return CataclysmDDA.SYSTEM_UI_MODE_SYSTEM_BARS;
            }
        }

        private int systemUiModeIndex(String mode) {
            if (CataclysmDDA.SYSTEM_UI_MODE_FULLSCREEN.equals(mode)) {
                return 1;
            } else if (CataclysmDDA.SYSTEM_UI_MODE_EDGE_TO_EDGE.equals(mode)) {
                return 2;
            }
            return 0;
        }

        @Override
        protected Boolean doInBackground(Void... params) {
            AssetManager assetManager = getAssets();
            String externalFilesDir = getExternalFilesDir(null).getPath();

            long deleteMillis;
            long copyMillis;
            try {
                // builds without the generated list fall back to walking the APK
                long phaseStart = SystemClock.elapsedRealtime();
                List<String> assetFiles = readAssetFileList(assetManager);
                if (assetFiles == null) {
                    assetFiles = new ArrayList<String>();
                    collectAssetFiles(assetManager, "data", assetFiles);
                    collectAssetFiles(assetManager, "gfx", assetFiles);
                    collectAssetFiles(assetManager, "lang", assetFiles);
                }
                totalFiles = assetFiles.size();
                countMillis = SystemClock.elapsedRealtime() - phaseStart;
                publishProgress(installedFiles, totalFiles);

                phaseStart = SystemClock.elapsedRealtime();
                // Clear out the old data if it exists (but preserve custom folders + files)
                deleteRecursive(assetManager, externalFilesDir, new File(externalFilesDir + "/data"));
                deleteRecursive(assetManager, externalFilesDir, new File(externalFilesDir + "/gfx"));
                deleteRecursive(assetManager, externalFilesDir, new File(externalFilesDir + "/lang"));
                deleteMillis = SystemClock.elapsedRealtime() - phaseStart;

                phaseStart = SystemClock.elapsedRealtime();
                // Install the new data over the top
                String lastFolder = "";
                for (String assetFile : assetFiles) {
                    File target = new File(externalFilesDir + "/" + assetFile);
                    String folder = target.getParent();
                    if (!folder.equals(lastFolder)) {
                        new File(folder).mkdirs();
                        lastFolder = folder;
                    }
                    copyAsset(assetManager, assetFile, target.getPath());
                }
                copyMillis = SystemClock.elapsedRealtime() - phaseStart;
            } catch(Exception e) {
                installationAlert.setMessage(e.getMessage());
                return false;
            }

            // Remember which version the installed data is
            PreferenceManager.getDefaultSharedPreferences(getApplicationContext()).edit().putString("installed", getVersionName()).commit();

            publishProgress(++installedFiles);
            Log.i(TAG, "Installed " + installedFiles + " files, " + copiedBytes + " bytes: count "
                  + countMillis + " ms, delete " + deleteMillis + " ms, copy " + copyMillis + " ms");
            return true;
        }

        void deleteRecursive(AssetManager assetManager, String externalFilesDir, File fileOrDirectory) {
            String parentFolder = fileOrDirectory.getParentFile().getName().toLowerCase();
            String fileOrDirectoryName = fileOrDirectory.getName().toLowerCase();
            if (fileOrDirectory.isDirectory()) {
                // Don't delete the folder if it is in the preserve folders list
                if (PRESERVE_FOLDERS.contains(fileOrDirectoryName))
                    return;

                // Don't delete the folder if its parent is in the preserve subfolders list, and it doesn't exist in the APK assets (so must be custom data)
                if (PRESERVE_SUBFOLDERS.contains(parentFolder) && !assetExists(assetManager, fileOrDirectory.getPath().substring(externalFilesDir.length()+1)))
                    return;

                for (File child : fileOrDirectory.listFiles())
                    deleteRecursive(assetManager, externalFilesDir, child);
            }
            else {
                // Don't delete the file if it's in the preserve files list
                if (PRESERVE_FILES.contains(fileOrDirectoryName))
                    return;
            }

            fileOrDirectory.delete();
        }

        // Returns true if an asset exists in the APK (either a directory or a file)
        // eg. assetExists("data/sound") or assetExists("data/font", "unifont.ttf") would both return true
        private boolean assetExists(AssetManager assetManager, String assetPath) {
            return assetExists(assetManager, assetPath, "");
        }

        private boolean assetExists(AssetManager assetManager, String assetPath, String assetName) {
            try {
                String[] files = assetManager.list(assetPath);
                if (assetName.isEmpty())
                    return files.length > 0; // folder exists
                for (String file : files) {
                    if (file.equalsIgnoreCase(assetName))
                        return true; // file exists
                }
                return false;
            } catch (Exception e) {
                e.printStackTrace();
                return false;
            }
        }

        // asset paths generated by generateAssetFileList at build time, or null if not
        // written
        private List<String> readAssetFileList(AssetManager assetManager) throws IOException {
            InputStream in;
            try {
                in = assetManager.open(ASSET_FILE_LIST);
            } catch (FileNotFoundException e) {
                return null;
            }
            List<String> files = new ArrayList<String>();
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(in, "UTF-8"))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    if (!line.isEmpty()) {
                        files.add(line);
                    }
                }
            }
            return files;
        }

        private void collectAssetFiles(AssetManager assetManager, String assetPath, List<String> out) throws Exception {
            for (String file : assetManager.list(assetPath)) {
                String filePath = assetPath + "/" + file;
                String[] children = assetManager.list(filePath);
                if (children.length == 0) {
                    out.add(filePath);
                } else {
                    collectAssetFiles(assetManager, filePath, out);
                }
            }
        }

        private boolean copyAsset(AssetManager assetManager, String fromAssetPath, String toPath) throws Exception {
            ++installedFiles;
            long now = SystemClock.elapsedRealtime();
            if (now - lastProgressMillis >= PROGRESS_INTERVAL_MILLIS) {
                lastProgressMillis = now;
                publishProgress(installedFiles);
            }
            InputStream in = null;
            OutputStream out = null;
            try {
              in = assetManager.open(fromAssetPath);
              new File(toPath).createNewFile();
              out = new FileOutputStream(toPath);
              copyFile(in, out);
              in.close();
              in = null;
              out.flush();
              out.close();
              out = null;
              return true;
            } catch(Exception e) {
                e.printStackTrace();
                throw e;
            }
        }

        private void copyFile(InputStream in, OutputStream out) throws IOException {
            int read;
            while((read = in.read(buffer)) != -1) {
              out.write(buffer, 0, read);
              copiedBytes += read;
            }
        }

        @Override
        protected void onProgressUpdate(Integer... values) {
            if (installDialog == null) {
                return;
            }
            if (values.length > 1) {
                installDialog.setIndeterminate(false);
                installDialog.setMax(values[1]);
            }
            installDialog.setProgress(values[0]);
        }

        @Override
        protected void onPostExecute(Boolean result) {
            removeDialog(INSTALL_DIALOG_ID);
            if(result) {
                settingsAlert.show();
            } else {
                installationAlert.show();
            }
        }
    }
}
