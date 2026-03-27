package com.cleverraven.cataclysmdda;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

import android.content.Context;
import android.view.accessibility.AccessibilityManager;
import android.accessibilityservice.AccessibilityService;
import android.accessibilityservice.AccessibilityServiceInfo;
import android.content.pm.ServiceInfo;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.widget.Toast;

public class CataclysmDDA_Helpers {
    public static List<AccessibilityServiceInfo> getEnabledAccessibilityServiceInfo(Context context) {
        AccessibilityManager am = (AccessibilityManager) context.getSystemService(Context.ACCESSIBILITY_SERVICE);
        List<AccessibilityServiceInfo> enabledServicesInfo = am.getEnabledAccessibilityServiceList(AccessibilityServiceInfo.FEEDBACK_ALL_MASK);
        return enabledServicesInfo;
    }

    public static String getEnabledAccessibilityServiceNames(Context context) {
        List<AccessibilityServiceInfo> enabledServicesInfo = getEnabledAccessibilityServiceInfo( context );
        String service_names = "";
        Set<String> false_positives = PreferenceManager.getDefaultSharedPreferences(context.getApplicationContext()).getStringSet("Accessibility Service Info False Positives", new HashSet<String>());
        for (AccessibilityServiceInfo enabledService : enabledServicesInfo) {
            ServiceInfo enabledServiceInfo = enabledService.getResolveInfo().serviceInfo;
            String service_name = enabledServiceInfo.name;
            if( !false_positives.contains( service_name ) ) {
                service_names = service_names + "\n" + service_name;
            }
        }
        return service_names;
    }

    public static void saveAccessibilityServiceInfoFalsePositives(Context context) {
        List<AccessibilityServiceInfo> enabledServicesInfo = getEnabledAccessibilityServiceInfo( context );
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context.getApplicationContext());
        // Purposeful copy to avoid some nonsense
        Set<String> false_positives = new HashSet<String>(preferences.getStringSet("Accessibility Service Info False Positives", new HashSet<String>()));
        for (AccessibilityServiceInfo enabledService : enabledServicesInfo) {
            ServiceInfo enabledServiceInfo = enabledService.getResolveInfo().serviceInfo;
            false_positives.add( enabledServiceInfo.name );
        }
        SharedPreferences.Editor editor = preferences.edit();
        editor.putStringSet("Accessibility Service Info False Positives", false_positives);
        editor.commit();        
    }
}
