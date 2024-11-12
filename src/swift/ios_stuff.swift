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
        let scenes = UIApplication.shared.connectedScenes
        let windowScene = scenes.first as? UIWindowScene
        let window = windowScene?.windows.first
        let safeareainsets = window?.safeAreaInsets
        // Yeah i know it's extremely dirty
        //iossetvisibledisplayframe(Int32(safeareainsets?.left ?? 9999), Int32(safeareainsets?.top ?? 9999), Int32(safeareainsets?.right ?? 9999), Int32(safeareainsets?.bottom ?? 9999))
        return super.application(application, didFinishLaunchingWithOptions: launchOptions)
    }
}