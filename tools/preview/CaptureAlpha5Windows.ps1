param(
    [Parameter(Mandatory)][int]$GmatPid,
    [Parameter(Mandatory)][string]$Destination
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
}
'@
[void][DesktopCapture]::SetProcessDPIAware()
$taskbar = [DesktopCapture]::FindWindow('Shell_TrayWnd', $null)
if ($taskbar -eq [IntPtr]::Zero -or -not [DesktopCapture]::IsWindowVisible($taskbar)) {
    throw 'The Windows Explorer taskbar is unavailable in this desktop session.'
}
$window = (Get-Process -Id $GmatPid).MainWindowHandle
if ($window -eq [IntPtr]::Zero) { throw 'GMAT has no visible top-level window.' }
[void][DesktopCapture]::ShowWindowAsync($window, 3)
[void][DesktopCapture]::SetForegroundWindow($window)
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
