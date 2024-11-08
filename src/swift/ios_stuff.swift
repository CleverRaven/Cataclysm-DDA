import Foundation
import GameController
import UIKit
import AVFoundation
import OSLog

extension UIWindow {
    static var current: UIWindow? {
        for scene in UIApplication.shared.connectedScenes {
            guard let windowScene = scene as? UIWindowScene else { continue }
            for window in windowScene.windows {
                if window.isKeyWindow { return window }
            }
        }
        return nil
    }
}

extension SDLUIKitDelegate {
    class func getAppDelegateClassName() -> String? {
        return NSStringFromClass(AppDelegate.self.self)
    }
}

extension UIScreen {
    static var current: UIScreen? {
        UIWindow.current?.screen
    }
}

public func iosGetDisplayDensity() -> Float {
    let scale = UIScreen.current?.nativeScale ?? 1.0
    return Float(scale)
}

public func isIOSKeyBoardAvailable() -> Bool {
    return GCKeyboard.coalesced != nil
}

public func vibrateDevice() {
    AudioServicesPlaySystemSound(kSystemSoundID_Vibrate)
}

class AppDelegate : SDLUIKitDelegate
{
    override func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool
    {
        os_log("Calling from SWIFT")
        return super.application(application, didFinishLaunchingWithOptions: launchOptions)
    }
}