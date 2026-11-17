param(
    [int]$GmatPid,
    [string]$Destination,
    [switch]$ConfigureHD,
    [switch]$MarsTargeting,
    [switch]$MarsTransfer
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class DesktopCapture {
    [DllImport("user32.dll")] public static extern bool ShowWindowAsync(IntPtr handle, int command);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr handle);
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string className, string name);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr handle);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint x, uint y, uint data, UIntPtr extra);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern bool EnumDisplaySettings(string device, int mode, ref DisplayMode data);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern int ChangeDisplaySettings(ref DisplayMode data, int flags);

    // DEVMODEW's display fields. The printer union and unused fields retain
    // their native byte offsets so the enumerated mode can be passed back.
    [StructLayout(LayoutKind.Explicit, Size = 220, CharSet = CharSet.Unicode)]
    private struct DisplayMode {
        [FieldOffset(68)] public ushort Size;
        [FieldOffset(72)] public uint Fields;
        [FieldOffset(168)] public uint BitsPerPixel;
        [FieldOffset(172)] public uint Width;
        [FieldOffset(176)] public uint Height;
        [FieldOffset(184)] public uint Frequency;
    }

    public static void SetFullHD() {
        var candidates = new System.Collections.Generic.List<DisplayMode>();
        for (int i = 0; ; ++i) {
            var mode = new DisplayMode { Size = 220 };
            if (!EnumDisplaySettings(null, i, ref mode)) break;
            if (mode.BitsPerPixel < 32) continue;
            System.Console.WriteLine("Display mode {0}x{1} @{2}", mode.Width, mode.Height, mode.Frequency);
            if (mode.Width >= 1920 && mode.Height >= 1080) candidates.Add(mode);
        }
        candidates.Sort((a, b) => ((long)a.Width*a.Height).CompareTo((long)b.Width*b.Height));
        foreach (var candidate in candidates) {
            var mode = candidate;
            mode.Fields = 0x00080000 | 0x00100000 | 0x00040000 | 0x00400000;
            int result = ChangeDisplaySettings(ref mode, 0);
            if (result == 0) return;
            System.Console.WriteLine("Could not set {0}x{1}: {2}", mode.Width, mode.Height, result);
        }
        throw new InvalidOperationException("No supported 1920x1080 or larger Windows display mode.");
    }

}
'@
[void][DesktopCapture]::SetProcessDPIAware()
if ($ConfigureHD) {
    $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    if ($bounds.Width -lt 1920 -or $bounds.Height -lt 1080) {
        [DesktopCapture]::SetFullHD()
    }
    exit
}
$taskbar = [DesktopCapture]::FindWindow('Shell_TrayWnd', $null)
if ($taskbar -eq [IntPtr]::Zero -or -not [DesktopCapture]::IsWindowVisible($taskbar)) {
    throw 'The Windows Explorer taskbar is unavailable in this desktop session.'
}
$window = (Get-Process -Id $GmatPid).MainWindowHandle
if ($window -eq [IntPtr]::Zero) { throw 'GMAT has no visible top-level window.' }
[void][DesktopCapture]::ShowWindowAsync($window, 3)
[void][DesktopCapture]::SetForegroundWindow($window)
if ($MarsTargeting -or $MarsTransfer) {
    Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes
    Start-Sleep -Seconds 1
    $root = [System.Windows.Automation.AutomationElement]::FromHandle($window)
    $tabCondition = [System.Windows.Automation.PropertyCondition]::new(
        [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
        [System.Windows.Automation.ControlType]::TabItem
    )
    $tabNames = if ($MarsTransfer) { @('OFI_SolarSystemView') } else { @('OFI_MarsView', 'Solver') }
    foreach ($prefix in $tabNames) {
        $tabs = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, $tabCondition)
        $tab = $tabs | Where-Object { $_.Current.Name.StartsWith($prefix) -and -not $_.Current.IsOffscreen } | Select-Object -First 1
        if (-not $tab) {
            $tabs | ForEach-Object { Write-Host $_.Current.Name }
            throw "Could not find workspace tab $prefix."
        }
        $rect = $tab.Current.BoundingRectangle
        [System.Windows.Forms.Cursor]::Position = [System.Drawing.Point]::new(
            [int]($rect.X + $rect.Width/2), [int]($rect.Y + $rect.Height/2))
        [DesktopCapture]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)
        [DesktopCapture]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 300
    }
    if ($MarsTargeting) {
        [System.Windows.Forms.SendKeys]::SendWait('%w')
        Start-Sleep -Milliseconds 300
        [System.Windows.Forms.SendKeys]::SendWait('t')
    }
}
$bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
[System.Windows.Forms.Cursor]::Position = [System.Drawing.Point]::new($bounds.Right - 2, $bounds.Bottom - 2)
Start-Sleep -Seconds 1
$image = [System.Drawing.Bitmap]::new($bounds.Width, $bounds.Height)
$graphics = [System.Drawing.Graphics]::FromImage($image)
try {
    $graphics.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
    $image.Save($Destination, [System.Drawing.Imaging.ImageFormat]::Png)
} finally {
    $graphics.Dispose()
    $image.Dispose()
}
