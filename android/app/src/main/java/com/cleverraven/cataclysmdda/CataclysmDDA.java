package com.cleverraven.cataclysmdda;

import org.libsdl.app.SDLActivity;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.Insets;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.os.Vibrator;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.widget.Toast;

public class CataclysmDDA extends SDLActivity {
    private static final String TAG = "CDDA";
    public static final String PREF_SYSTEM_UI_MODE = "Android system UI mode";
    public static final String PREF_FORCE_FULLSCREEN = "Force fullscreen";
    public static final String SYSTEM_UI_MODE_SYSTEM_BARS = "system_bars";
    public static final String SYSTEM_UI_MODE_FULLSCREEN = "fullscreen";
    public static final String SYSTEM_UI_MODE_EDGE_TO_EDGE = "edge_to_edge";

    private NativeUI nativeUI = new NativeUI(CataclysmDDA.this);
    private int lastImeLeft = -1;
    private int lastImeTop = -1;
    private int lastImeRight = -1;
    private int lastImeBottom = -1;
    private boolean lastImeVisible = false;

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
        setImeInsetListener();
        applySystemUiMode();
    }

    @Override
    protected void onResume() {
        super.onResume();
        applySystemUiMode();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            applySystemUiMode();
        }
    }

    private String normalizeSystemUiMode(String mode) {
        if (SYSTEM_UI_MODE_FULLSCREEN.equals(mode) || SYSTEM_UI_MODE_EDGE_TO_EDGE.equals(mode)) {
            return mode;
        }
        return SYSTEM_UI_MODE_SYSTEM_BARS;
    }

    private String getStoredSystemUiMode() {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        String mode;
        if (preferences.contains(PREF_SYSTEM_UI_MODE)) {
            mode = normalizeSystemUiMode(preferences.getString(PREF_SYSTEM_UI_MODE, SYSTEM_UI_MODE_SYSTEM_BARS));
        } else {
            mode = preferences.getBoolean(PREF_FORCE_FULLSCREEN, false)
                ? SYSTEM_UI_MODE_EDGE_TO_EDGE
                : SYSTEM_UI_MODE_SYSTEM_BARS;
        }
        preferences.edit().putString(PREF_SYSTEM_UI_MODE, mode).apply();
        return mode;
    }

    private void applySystemUiMode() {
        applySystemUiMode(getStoredSystemUiMode());
    }

    private void applySystemUiMode(String rawMode) {
        String mode = normalizeSystemUiMode(rawMode);
        boolean hideSystemBars = !SYSTEM_UI_MODE_SYSTEM_BARS.equals(mode);
        boolean edgeToEdge = SYSTEM_UI_MODE_EDGE_TO_EDGE.equals(mode);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            getWindow().setDecorFitsSystemWindows(!edgeToEdge);
            WindowInsetsController controller = getWindow().getInsetsController();
            if (controller != null) {
                if (hideSystemBars) {
                    controller.hide(WindowInsets.Type.systemBars());
                    controller.setSystemBarsBehavior(
                        WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                } else {
                    controller.show(WindowInsets.Type.systemBars());
                }
            }
        } else {
            View decor = getWindow().getDecorView();
            if (hideSystemBars) {
                decor.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_FULLSCREEN);
            } else {
                decor.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
            }
        }

        if (hideSystemBars) {
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        } else {
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    private void setImeInsetListener() {
        final View decor = getWindow().getDecorView();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            decor.setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener() {
                @Override
                public WindowInsets onApplyWindowInsets(View view, WindowInsets insets) {
                    boolean imeVisible = insets.isVisible(WindowInsets.Type.ime());
                    Insets imeInsets = insets.getInsets(WindowInsets.Type.ime());
                    notifyImeInsetsChanged(
                        imeInsets.left,
                        imeInsets.top,
                        Math.max(0, view.getWidth() - imeInsets.right),
                        Math.max(0, view.getHeight() - imeInsets.bottom),
                        imeVisible);
                    return insets;
                }
            });
            decor.requestApplyInsets();
        } else {
            decor.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    Rect visibleFrame = new Rect();
                    decor.getWindowVisibleDisplayFrame(visibleFrame);
                    int rootHeight = decor.getRootView().getHeight();
                    boolean imeVisible = rootHeight - visibleFrame.bottom > rootHeight / 5;
                    notifyImeInsetsChanged(
                        visibleFrame.left,
                        visibleFrame.top,
                        visibleFrame.right,
                        visibleFrame.bottom,
                        imeVisible);
                }
            });
        }
    }

    private void notifyImeInsetsChanged(int left, int top, int right, int bottom, boolean visible) {
        if (lastImeLeft == left && lastImeTop == top && lastImeRight == right &&
                lastImeBottom == bottom && lastImeVisible == visible) {
            return;
        }
        lastImeLeft = left;
        lastImeTop = top;
        lastImeRight = right;
        lastImeBottom = bottom;
        lastImeVisible = visible;
        try {
            onNativeImeInsetsChanged(left, top, right, bottom, visible);
        } catch(UnsatisfiedLinkError e) {
            // The Activity can receive early inset callbacks before native startup.
        }
    }

    private static native void onNativeImeInsetsChanged(
        int left, int top, int right, int bottom, boolean visible);

    public void setSystemUiMode(final String mode) {
        final String normalizedMode = normalizeSystemUiMode(mode);
        PreferenceManager.getDefaultSharedPreferences(getApplicationContext())
            .edit()
            .putString(PREF_SYSTEM_UI_MODE, normalizedMode)
            .apply();
        try {
            runOnUiThread(new Runnable() {
                public void run() {
                    applySystemUiMode(normalizedMode);
                }
            });
        } catch(Exception e) {
            System.err.println(e.getMessage());
        }
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

    public String getDefaultStringSetting(final String settingsName, String defaultValue) {
        if (PREF_SYSTEM_UI_MODE.equals(settingsName)) {
            return getStoredSystemUiMode();
        }
        String setting = PreferenceManager.getDefaultSharedPreferences(getApplicationContext())
            .getString(settingsName, defaultValue);
        return setting != null ? setting : defaultValue;
    }

    public String getSystemLang() {
        return getResources().getConfiguration().locale.toLanguageTag().replace('-', '_');
    }

    public NativeUI getNativeUI() {
        return nativeUI;
    }
}
