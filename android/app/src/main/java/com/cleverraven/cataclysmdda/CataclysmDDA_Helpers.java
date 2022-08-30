package com.cleverraven.cataclysmdda;

import java.util.List;

import android.content.Context;
import android.view.accessibility.AccessibilityManager;
import android.accessibilityservice.AccessibilityService;
import android.accessibilityservice.AccessibilityServiceInfo;
import android.content.pm.ServiceInfo;
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
        for (AccessibilityServiceInfo enabledService : enabledServicesInfo) {
            ServiceInfo enabledServiceInfo = enabledService.getResolveInfo().serviceInfo;
            String service_name = enabledServiceInfo.name;
            service_names = service_names + "\n" + service_name;
        }
        return service_names;
    }
}
