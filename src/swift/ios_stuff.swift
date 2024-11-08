import Foundation
import GameController
import UIKit
import AVFoundation
import OSLog

//https://stackoverflow.com/a/76728094
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

extension UIScreen {
    static var current: UIScreen? {
        UIWindow.current?.screen
    }
}

extension UIApplication {
    var keyWindow: UIWindow? {
        // Get connected scenes
        return self.connectedScenes
            // Keep only active scenes, onscreen and visible to the user
            .filter { $0.activationState == .foregroundActive }
            // Keep only the first `UIWindowScene`
            .first(where: { $0 is UIWindowScene })
            // Get its associated windows
            .flatMap({ $0 as? UIWindowScene })?.windows
            // Finally, keep only the key window
            .first(where: \.isKeyWindow)
    }   
}

extension SDLUIKitDelegate {
    class func getAppDelegateClassName() -> String? {
        return NSStringFromClass(AppDelegate.self.self)
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

    func viewDidLoad() {
        super.viewDidLoad()
        let safeareainsets = UIApplication.keywindow.safeAreaInsets
        iossetvisibledisplayframe(safeareainsets.left, safeareainsets.top, safeareainsets.right, safeareainsets.bottom)
    }
}