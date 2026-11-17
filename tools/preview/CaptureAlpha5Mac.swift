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
        if CGDisplaySetDisplayMode(display, mode, nil) == .success { return }
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

func children(_ element: AXUIElement) -> [AXUIElement] {
    return attribute(element, kAXChildrenAttribute) as? [AXUIElement] ?? []
}

func locate(_ element: AXUIElement, role: String, prefix: String) -> AXUIElement? {
    let label = [kAXTitleAttribute, kAXDescriptionAttribute, kAXIdentifierAttribute]
        .compactMap { attribute(element, $0) as? String }.joined(separator: " ")
    if attribute(element, kAXRoleAttribute) as? String == role && label.hasPrefix(prefix) { return element }
    for child in children(element) {
        if let result = locate(child, role: role, prefix: prefix) { return result }
    }
    return nil
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

func moveAndResize(_ element: AXUIElement, to target: CGRect) {
    guard let current = rect(element) else { fail("Missing initial child geometry") }
    if abs(current.minX-target.minX) > 3 || abs(current.minY-target.minY) > 3 {
        let from = CGPoint(x: current.midX, y: current.minY + 10)
        let to = CGPoint(x: from.x + target.minX - current.minX, y: target.minY + 10)
        drag(from, to)
    }
    guard let moved = rect(element) else { fail("Missing moved child geometry") }
    let from = CGPoint(x: moved.maxX-2, y: moved.maxY-2)
    let to = CGPoint(x: moved.minX + target.width-2, y: moved.minY + target.height-2)
    if hypot(from.x-to.x, from.y-to.y) > 4 {
        drag(from, to)
    }
    guard let finished = rect(element) else { fail("Missing final child geometry") }
    print("Child frame \(current) -> \(finished); requested \(target)")
    if abs(finished.minX-target.minX) > 12 || abs(finished.minY-target.minY) > 12 ||
        abs(finished.width-target.width) > 20 || abs(finished.height-target.height) > 20 {
        fail("Could not arrange the visible GMAT child window")
    }
}

func layoutMars(pid: pid_t) {
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

    guard let solver = locate(main, role: kAXWindowRole, prefix: "Solver"),
          let plot = locate(main, role: kAXWindowRole, prefix: "OFI_MarsView"),
          let solverRect = rect(solver), let appRect = rect(main) else {
        fail("The converged solver and Mars plot were not exposed by Qt accessibility")
    }
    // The native macOS Window menu has no items until first opened, so
    // arrange the two MDI children via normal title bar and edge drags.
    let left = solverRect.minX
    let top = solverRect.minY
    let right = appRect.maxX - 7
    let bottom = appRect.maxY - 188
    let half = floor((right-left)/2)
    moveAndResize(solver, to: CGRect(x: left, y: top, width: half-2, height: bottom-top))
    // Bring the partially obscured Mars plot forward using its normal
    // workspace tab, then restore the child so its frame can be dragged.
    guard let tab = locate(main, role: kAXRadioButtonRole, prefix: "OFI_MarsView"),
          let tabRect = rect(tab) else { fail("No Mars plot workspace tab") }
    let tabPoint = CGPoint(x: tabRect.midX, y: tabRect.midY)
    mouse(.leftMouseDown, tabPoint)
    mouse(.leftMouseUp, tabPoint)
    usleep(500000)
    guard let restore = locate(main, role: kAXButtonRole, prefix: "Restore window"),
          let restoreRect = rect(restore) else { fail("No Mars plot restore control") }
    let restorePoint = CGPoint(x: restoreRect.midX, y: restoreRect.midY)
    mouse(.leftMouseDown, restorePoint)
    mouse(.leftMouseUp, restorePoint)
    usleep(500000)
    moveAndResize(plot, to: CGRect(x: left+half, y: top, width: half, height: bottom-top))
    CGWarpMouseCursorPosition(CGPoint(x: screen.frame.maxX-2, y: screen.frame.maxY-3))
}

let args = CommandLine.arguments.dropFirst()
if args.first == "configure" {
    configureFullHD()
} else if args.count == 2, args.first == "arrange", let pid = Int32(args.last!) {
    layoutMars(pid: pid)
} else {
    fail("Usage: CaptureAlpha5Mac configure | arrange PID")
}
