import AppKit
import ApplicationServices
import CoreGraphics
import Foundation

func fail(_ message: String) -> Never {
    FileHandle.standardError.write(Data((message + "\n").utf8))
    exit(1)
}

func configureFullHD() {
    let display = CGMainDisplayID()
    guard let current = CGDisplayCopyDisplayMode(display) else { fail("No macOS desktop display") }
    if current.width >= 1920 && current.height >= 1080 { return }
    let options = [kCGDisplayShowDuplicateLowResolutionModes as String: true] as CFDictionary
    let modes = CGDisplayCopyAllDisplayModes(display, options) as? [CGDisplayMode] ?? []
    for mode in modes {
        print("macOS mode \(mode.width)x\(mode.height) pixels \(mode.pixelWidth)x\(mode.pixelHeight)")
    }
    let usable = modes.filter { $0.width >= 1920 && $0.height >= 1080 }
        .sorted { $0.pixelWidth * $0.pixelHeight < $1.pixelWidth * $1.pixelHeight }
    for mode in usable {
        var configuration: CGDisplayConfigRef?
        guard CGBeginDisplayConfiguration(&configuration) == .success else { continue }
        if CGConfigureDisplayWithDisplayMode(configuration, display, mode, nil) != .success {
            CGCancelDisplayConfiguration(configuration)
            continue
        }
        if CGCompleteDisplayConfiguration(configuration, .forSession) == .success { return }
    }
    fail("No supported 1920x1080 or larger macOS desktop mode")
}

func attribute(_ object: AXUIElement, _ name: String) -> CFTypeRef? {
    var result: CFTypeRef?
    guard AXUIElementCopyAttributeValue(object, name as CFString, &result) == .success else { return nil }
    return result
}

func position(_ element: AXUIElement) -> CGPoint? {
    guard let value = attribute(element, kAXPositionAttribute), CFGetTypeID(value) == AXValueGetTypeID() else {
        return nil
    }
    var point = CGPoint.zero
    guard AXValueGetValue(value as! AXValue, .cgPoint, &point) else { return nil }
    return point
}

func rect(_ element: AXUIElement) -> CGRect? {
    guard let point = position(element),
          let value = attribute(element, kAXSizeAttribute), CFGetTypeID(value) == AXValueGetTypeID() else {
        return nil
    }
    var size = CGSize.zero
    guard AXValueGetValue(value as! AXValue, .cgSize, &size) else { return nil }
    return CGRect(origin: point, size: size)
}

func mouse(_ type: CGEventType, _ point: CGPoint) {
    guard let event = CGEvent(mouseEventSource: nil, mouseType: type,
                              mouseCursorPosition: point, mouseButton: .left) else {
        fail("Could not make a macOS pointer event")
    }
    event.post(tap: .cghidEventTap)
}

func drag(_ start: CGPoint, _ finish: CGPoint) {
    mouse(.mouseMoved, start)
    usleep(60000)
    mouse(.leftMouseDown, start)
    for step in 1...12 {
        let fraction = CGFloat(step) / 12.0
        mouse(.leftMouseDragged, CGPoint(x: start.x + (finish.x - start.x) * fraction,
                                        y: start.y + (finish.y - start.y) * fraction))
        usleep(25000)
    }
    mouse(.leftMouseUp, finish)
    usleep(280000)
}

func fitMainWindow(pid: pid_t) {
    guard AXIsProcessTrusted() else { fail("macOS runner has not granted accessibility access") }
    guard let app = NSRunningApplication(processIdentifier: pid), let screen = NSScreen.screens.first else {
        fail("No running GMAT app or macOS display")
    }
    _ = app.activate(options: [.activateIgnoringOtherApps])
    usleep(300000)
    let axApp = AXUIElementCreateApplication(pid)
    guard let windows = attribute(axApp, kAXWindowsAttribute) as? [AXUIElement], let main = windows.first else {
        fail("No accessible GMAT top-level window")
    }
    let visible = screen.visibleFrame
    var origin = CGPoint(x: visible.minX, y: screen.frame.maxY-visible.maxY)
    var size = visible.size
    let status = AXUIElementSetAttributeValue(main, kAXPositionAttribute as CFString,
                                              AXValueCreate(.cgPoint, &origin)!)
    guard status == .success && AXUIElementSetAttributeValue(main, kAXSizeAttribute as CFString,
              AXValueCreate(.cgSize, &size)!) == .success else { fail("Cannot fit GMAT above the Dock") }
    usleep(800000)

    guard let frame = rect(main), frame.width >= 1880, frame.height >= 940 else {
        fail("GMAT window did not expand to the HD desktop above the Dock")
    }
    print("GMAT frame above Dock: \(frame)")
}

func extendSolverTable() {
    // The shipped Qt solver starts with an 860x320 child at the top-left of
    // the workspace. Drag its bottom corner down to the message pane while
    // leaving the Mars plot and its mission clock alone.
    guard let screen = NSScreen.screens.first else { fail("No desktop") }
    let bottom = screen.frame.maxY - screen.visibleFrame.minY
    drag(CGPoint(x: 1127, y: 438), CGPoint(x: 1127, y: bottom - 191))
    CGWarpMouseCursorPosition(CGPoint(x: screen.frame.maxX-2, y: screen.frame.maxY-3))
}

func showTransferView() {
    guard let screen = NSScreen.screens.first, screen.frame.width == 1920 && screen.frame.height == 1080 else {
        fail("The matching transfer tab position expects the runner's 1920x1080 desktop")
    }
    // Fresh settings and the fitted main window place the Sun-centered
    // view tab in this position on both macOS runner images.
    let tab = CGPoint(x: 593, y: 109)
    mouse(.mouseMoved, tab)
    usleep(80000)
    mouse(.leftMouseDown, tab)
    mouse(.leftMouseUp, tab)
    usleep(350000)
    CGWarpMouseCursorPosition(CGPoint(x: screen.frame.maxX-2, y: screen.frame.maxY-3))
}

let args = CommandLine.arguments.dropFirst()
if args.first == "configure" {
    configureFullHD()
} else if args.count == 2, args.first == "fit", let pid = Int32(args.last!) {
    fitMainWindow(pid: pid)
} else if args.first == "solver" {
    extendSolverTable()
} else if args.first == "transfer" {
    showTransferView()
} else {
    fail("Usage: CaptureAlpha5Mac configure | fit PID | solver | transfer")
}
