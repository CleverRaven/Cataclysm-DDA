import Foundation
import GameController
import UIKit
import AVFoundation

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
