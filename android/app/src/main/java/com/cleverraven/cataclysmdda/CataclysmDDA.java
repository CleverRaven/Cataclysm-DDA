package com.cleverraven.cataclysmdda;

import org.libsdl.app.SDLActivity;

import android.content.Context;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.os.Vibrator;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.widget.Toast;

public class CataclysmDDA extends SDLActivity {
    private static final String TAG = "CDDA";

    private NativeUI nativeUI = new NativeUI(CataclysmDDA.this);

    // libmain.so must load first so cata_allocator binds before SDL's malloc.
    // SDL3 dlsym's SDL_main from getMainSharedObject(), which we point at libmain.so.
    @Override
    protected String[] getLibraries() {
        return new String[] {
            "main",
            "SDL3",
            "SDL3_image",
            "SDL3_mixer",
            "SDL3_ttf",
        };
    }

    @Override
    protected String getMainSharedObject() {
        return getApplicationInfo().nativeLibraryDir + "/libmain.so";
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (mLayout != null) {
            mLayout.setVisibility(View.INVISIBLE);
        }
        applyImmersive();
    }

    @Override
    protected void onResume() {
        super.onResume();
        applyImmersive();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            applyImmersive();
        }
    }

    private void applyImmersive() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            WindowInsetsController controller = getWindow().getInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.systemBars());
                controller.setSystemBarsBehavior(
                    WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            View decor = getWindow().getDecorView();
            decor.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN);
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    public void vibrate(int duration) {
        try {
            Vibrator v = (Vibrator)getSystemService(Context.VIBRATOR_SERVICE);
            v.vibrate(duration);
        } catch(Exception e) {
            System.err.println(e.getMessage());
        }
    }

    public void toast(final String message) {
        try {
            runOnUiThread(new Runnable() {
                public void run() {
                    Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT).show();
                }
            });
        } catch(Exception e) {
            System.err.println(e.getMessage());
        }
    }

    private boolean isHardwareKeyboardAvailable() {
        return getResources().getConfiguration().keyboard == Configuration.KEYBOARD_QWERTY;
    }

    private float getDisplayDensity() {
        return getResources().getDisplayMetrics().density;
    }

    public void show_sdl_surface() {
        try {
            runOnUiThread(new Runnable() {
                public void run() {
                    if (mLayout != null) {
                        mLayout.setVisibility(View.VISIBLE);
                    }
                }
            });
        } catch(Exception e) {
            System.err.println(e.getMessage());
        }
    }

    public boolean getDefaultSetting(final String settingsName, boolean defaultValue) {
        return PreferenceManager.getDefaultSharedPreferences(getApplicationContext()).getBoolean(settingsName, defaultValue);
    }

    public String getSystemLang() {
        return getResources().getConfiguration().locale.toLanguageTag().replace('-', '_');
    }

    public NativeUI getNativeUI() {
        return nativeUI;
    }
}
