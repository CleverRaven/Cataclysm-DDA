package com.cleverraven.cataclysmdda;

import org.libsdl.app.SDLActivity;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
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
    public static final String PREF_SCREEN_ORIENTATION = "Android screen orientation";
    public static final String SCREEN_ORIENTATION_LANDSCAPE = "landscape";
    public static final String SCREEN_ORIENTATION_PORTRAIT = "portrait";
    public static final String SCREEN_ORIENTATION_AUTO = "auto";

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
        applyStoredScreenOrientation(this);
        setImeInsetListener();
        applySystemUiMode();
    }

    @Override
    protected void onResume() {
        super.onResume();
        applyStoredScreenOrientation(this);
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
                    reportOuterSize();
                    reportSafeArea(insets);
                    boolean imeVisible = insets.isVisible(WindowInsets.Type.ime());
                    Insets imeInsets = insets.getInsets(WindowInsets.Type.ime());
                    int[] surfaceOrigin = new int[2];
                    if (mSurface != null) {
                        mSurface.getLocationInWindow(surfaceOrigin);
                    }
                    notifyImeInsetsChanged(
                        imeInsets.left,
                        imeInsets.top,
                        Math.max(0, view.getWidth() - imeInsets.right),
                        Math.max(0, view.getHeight() - imeInsets.bottom),
                        surfaceOrigin,
                        imeVisible);
                    return insets;
                }
            });
            decor.requestApplyInsets();
            if (mSurface != null) {
                mSurface.addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
                    @Override
                    public void onLayoutChange(View v, int left, int top, int right, int bottom,
                                               int oldLeft, int oldTop, int oldRight, int oldBottom) {
                        reportOuterSize();
                        WindowInsets insets = decor.getRootWindowInsets();
                        if (insets != null) {
                            reportSafeArea(insets);
                        }
                    }
                });
            }
        } else {
            decor.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    reportOuterSize();
                    Rect visibleFrame = new Rect();
                    decor.getWindowVisibleDisplayFrame(visibleFrame);
                    int rootHeight = decor.getRootView().getHeight();
                    boolean imeVisible = rootHeight - visibleFrame.bottom > rootHeight / 5;
                    // visible frame is in screen coordinates
                    int[] surfaceOrigin = new int[2];
                    if (mSurface != null) {
                        mSurface.getLocationOnScreen(surfaceOrigin);
                    }
                    notifyImeInsetsChanged(
                        visibleFrame.left,
                        visibleFrame.top,
                        visibleFrame.right,
                        visibleFrame.bottom,
                        surfaceOrigin,
                        imeVisible);
                }
            });
        }
    }

    private void notifyImeInsetsChanged(int frameLeft, int frameTop, int frameRight, int frameBottom,
                                        int[] surfaceOrigin, boolean visible) {
        int[] scaled = scaleToSurface(frameLeft, frameTop, frameRight, frameBottom, surfaceOrigin);
        if (scaled == null) {
            return;
        }
        int left = scaled[0];
        int top = scaled[1];
        int right = scaled[2];
        int bottom = scaled[3];
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

    private static native void onNativeSafeAreaChanged(int left, int top, int right, int bottom);

    private static native void onNativeOuterSizeChanged(int width, int height);

    private int lastOuterWidth = -1;
    private int lastOuterHeight = -1;

    // whole-window size in pixels, system bars included. text size is picked from
    // it, so bars that inset the surface in one orientation only don't change it on
    // rotation
    private void reportOuterSize() {
        int width;
        int height;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Rect bounds = getWindowManager().getCurrentWindowMetrics().getBounds();
            width = bounds.width();
            height = bounds.height();
        } else {
            View decor = getWindow().getDecorView();
            width = decor.getWidth();
            height = decor.getHeight();
        }
        // zero while the window is still being laid out
        if (width <= 0 || height <= 0 || (width == lastOuterWidth && height == lastOuterHeight)) {
            return;
        }
        try {
            onNativeOuterSizeChanged(width, height);
        } catch(UnsatisfiedLinkError e) {
            // early layout callbacks can arrive before native startup; resend later
            return;
        }
        lastOuterWidth = width;
        lastOuterHeight = height;
    }

    // ten-thousandths of the surface size; native reader uses the same scale
    private static final int SAFE_AREA_SCALE = 10000;
    private int lastSafeLeft = -1;
    private int lastSafeTop = -1;
    private int lastSafeRight = -1;
    private int lastSafeBottom = -1;
    // null until the first report; Insets needs API 29, so no Insets.NONE here
    private Insets lastNavigationInsets = null;

    // in surface units, so native code needs no Java pixel sizes or origins
    private int[] scaleToSurface(int left, int top, int right, int bottom, int[] surfaceOrigin) {
        if (mSurface == null) {
            return null;
        }
        int surfaceWidth = mSurface.getWidth();
        int surfaceHeight = mSurface.getHeight();
        if (surfaceWidth <= 0 || surfaceHeight <= 0) {
            return null;
        }
        left = Math.max(0, left - surfaceOrigin[0]);
        top = Math.max(0, top - surfaceOrigin[1]);
        right = Math.min(surfaceWidth, right - surfaceOrigin[0]);
        bottom = Math.min(surfaceHeight, bottom - surfaceOrigin[1]);
        if (right < left || bottom < top) {
            return null;
        }
        return new int[] {
            (int)((long)left * SAFE_AREA_SCALE / surfaceWidth),
            (int)((long)top * SAFE_AREA_SCALE / surfaceHeight),
            (int)((long)right * SAFE_AREA_SCALE / surfaceWidth),
            (int)((long)bottom * SAFE_AREA_SCALE / surfaceHeight)
        };
    }

    // report the part of the SDL surface clear of the camera cutout and visible
    // system bars. gesture strips catch edge swipes, not taps, so the game may draw
    // under them
    private void reportSafeArea(WindowInsets insets) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R || mSurface == null) {
            return;
        }
        View decor = getWindow().getDecorView();
        // navigation bar can appear only while the keyboard is up; keep its
        // no-keyboard layout so opening the keyboard doesn't move the view
        Insets navigation;
        if (insets.isVisible(WindowInsets.Type.ime())) {
            navigation = lastNavigationInsets != null ? lastNavigationInsets : Insets.NONE;
        } else {
            navigation = insets.getInsets(WindowInsets.Type.navigationBars());
            lastNavigationInsets = navigation;
        }
        Insets safe = Insets.max(navigation, insets.getInsets(
            WindowInsets.Type.statusBars() | WindowInsets.Type.captionBar() |
            WindowInsets.Type.displayCutout()));
        int[] origin = new int[2];
        mSurface.getLocationInWindow(origin);
        int[] scaled = scaleToSurface(safe.left, safe.top, decor.getWidth() - safe.right,
                                      decor.getHeight() - safe.bottom, origin);
        if (scaled == null) {
            return;
        }
        int scaledLeft = scaled[0];
        int scaledTop = scaled[1];
        int scaledRight = scaled[2];
        int scaledBottom = scaled[3];
        if (scaledLeft == lastSafeLeft && scaledTop == lastSafeTop &&
                scaledRight == lastSafeRight && scaledBottom == lastSafeBottom) {
            return;
        }
        try {
            onNativeSafeAreaChanged(scaledLeft, scaledTop, scaledRight, scaledBottom);
        } catch(UnsatisfiedLinkError e) {
            // early inset callbacks can arrive before native startup; resend later
            return;
        }
        lastSafeLeft = scaledLeft;
        lastSafeTop = scaledTop;
        lastSafeRight = scaledRight;
        lastSafeBottom = scaledBottom;
    }

    static int requestedOrientationFor(String mode) {
        if (SCREEN_ORIENTATION_PORTRAIT.equals(mode)) {
            return ActivityInfo.SCREEN_ORIENTATION_USER_PORTRAIT;
        }
        if (SCREEN_ORIENTATION_AUTO.equals(mode)) {
            return ActivityInfo.SCREEN_ORIENTATION_FULL_USER;
        }
        return ActivityInfo.SCREEN_ORIENTATION_USER_LANDSCAPE;
    }

    static void applyStoredScreenOrientation(Activity activity) {
        String mode = PreferenceManager.getDefaultSharedPreferences(activity.getApplicationContext())
            .getString(PREF_SCREEN_ORIENTATION, SCREEN_ORIENTATION_LANDSCAPE);
        activity.setRequestedOrientation(requestedOrientationFor(mode));
    }

    // SDL asks for orientation from its hint and window shape; the game option
    // decides instead
    @Override
    public void setOrientationBis(int w, int h, boolean resizable, String hint) {
        applyStoredScreenOrientation(this);
    }

    public void setScreenOrientation(final String mode) {
        PreferenceManager.getDefaultSharedPreferences(getApplicationContext())
            .edit()
            .putString(PREF_SCREEN_ORIENTATION, mode)
            .apply();
        try {
            runOnUiThread(new Runnable() {
                public void run() {
                    setRequestedOrientation(requestedOrientationFor(mode));
                }
            });
        } catch(Exception e) {
            System.err.println(e.getMessage());
        }
    }

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
