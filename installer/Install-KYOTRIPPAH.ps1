# KYOTRIPPAH 0.5.1 installer. No console. Builds into the folder you launched from.
$ErrorActionPreference = "Stop"
# Ask for admin before the window is shown.
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    $scriptPath = $MyInvocation.MyCommand.Path
    if (-not $scriptPath) { $scriptPath = $PSCommandPath }
    Start-Process -FilePath "powershell.exe" -Verb RunAs -ArgumentList "-NoProfile -ExecutionPolicy Bypass -STA -WindowStyle Hidden -File `"$scriptPath`""
    exit 0
}
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$RunDir = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path (Join-Path $RunDir "Project\CMakeLists.txt"))) {
    $here = Split-Path -Parent $MyInvocation.MyCommand.Path
    $RunDir = Split-Path -Parent $here
}
$Project = Join-Path $RunDir "Project"
$BuildDir = Join-Path $RunDir "build"
$LocalOut = Join-Path $RunDir "Installed-VST3"
$LogFile = Join-Path $env:USERPROFILE "Downloads\KYOTRIPPAH-install-log.txt"
$script:logLines = New-Object System.Collections.Generic.List[string]
$script:installed = $false

function Add-Log([string]$line) {
    $stamp = (Get-Date).ToString("HH:mm:ss")
    $row = "$stamp  $line"
    $script:logLines.Add($row)
    if ($script:logBox) {
        $script:logBox.AppendText($row + "`r`n")
        $script:logBox.SelectionStart = $script:logBox.Text.Length
        $script:logBox.ScrollToCaret()
    }
    [System.Windows.Forms.Application]::DoEvents()
}

function Save-Log {
    $dir = Split-Path -Parent $LogFile
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    $script:logLines -join "`r`n" | Set-Content -Path $LogFile -Encoding UTF8
}

function Remove-Log {
    if (Test-Path $LogFile) { Remove-Item -LiteralPath $LogFile -Force -ErrorAction SilentlyContinue }
}

$bg = [System.Drawing.Color]::FromArgb(255, 14, 12, 20)
$panel = [System.Drawing.Color]::FromArgb(255, 26, 22, 36)
$accent = [System.Drawing.Color]::FromArgb(255, 199, 125, 255)
$ink = [System.Drawing.Color]::FromArgb(255, 240, 230, 255)
$muted = [System.Drawing.Color]::FromArgb(255, 138, 122, 168)

$form = New-Object System.Windows.Forms.Form
$form.Text = "KYOTRIPPAH 0.5.1"
$form.Size = New-Object System.Drawing.Size(720, 580)
$form.StartPosition = "CenterScreen"
$form.FormBorderStyle = "FixedSingle"
$form.MaximizeBox = $false
$form.BackColor = $bg
$form.ForeColor = $ink
$form.Font = New-Object System.Drawing.Font("Segoe UI", 10)

$form.Add_Paint({
    param($sender, $e)
    $g = $e.Graphics
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $face = New-Object System.Drawing.Rectangle(16, 16, 672, 520)
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        $face, [System.Drawing.Color]::FromArgb(255, 36, 30, 52), $panel, 45)
    $g.FillRectangle($brush, $face)
    $pen = New-Object System.Drawing.Pen($accent, 1.4)
    $g.DrawRectangle($pen, $face)
    $screw = New-Object System.Drawing.SolidBrush($muted)
    foreach ($p in @(@(28,28), @(668,28), @(28,516), @(668,516))) {
        $g.FillEllipse($screw, $p[0], $p[1], 8, 8)
    }
    $g.FillEllipse((New-Object System.Drawing.SolidBrush($accent)), 640, 36, 10, 10)
})

$title = New-Object System.Windows.Forms.Label
$title.Text = "KYOTRIPPAH"
$title.Font = New-Object System.Drawing.Font("Segoe UI", 20, [System.Drawing.FontStyle]::Bold)
$title.ForeColor = $accent
$title.BackColor = [System.Drawing.Color]::Transparent
$title.Location = New-Object System.Drawing.Point(40, 36)
$title.Size = New-Object System.Drawing.Size(400, 36)
$form.Controls.Add($title)

$sub = New-Object System.Windows.Forms.Label
$sub.Text = "HARDWARE INSTALLER  -  BUILD  0.5.1"
$sub.ForeColor = $muted
$sub.BackColor = [System.Drawing.Color]::Transparent
$sub.Location = New-Object System.Drawing.Point(42, 74)
$sub.Size = New-Object System.Drawing.Size(460, 22)
$form.Controls.Add($sub)

$status = New-Object System.Windows.Forms.Label
$status.Text = "Installs into this folder, then copies into the VST3 folder."
$status.ForeColor = $ink
$status.BackColor = [System.Drawing.Color]::Transparent
$status.Location = New-Object System.Drawing.Point(42, 102)
$status.Size = New-Object System.Drawing.Size(620, 36)
$form.Controls.Add($status)

$logBox = New-Object System.Windows.Forms.TextBox
$logBox.Multiline = $true
$logBox.ReadOnly = $true
$logBox.ScrollBars = "Vertical"
$logBox.BackColor = [System.Drawing.Color]::FromArgb(255, 10, 8, 16)
$logBox.ForeColor = $ink
$logBox.BorderStyle = "FixedSingle"
$logBox.Font = New-Object System.Drawing.Font("Consolas", 9)
$logBox.Location = New-Object System.Drawing.Point(42, 148)
$logBox.Size = New-Object System.Drawing.Size(620, 268)
$form.Controls.Add($logBox)
$script:logBox = $logBox

$bar = New-Object System.Windows.Forms.ProgressBar
$bar.Location = New-Object System.Drawing.Point(42, 428)
$bar.Size = New-Object System.Drawing.Size(620, 16)
$bar.Maximum = 100
$form.Controls.Add($bar)

$installBtn = New-Object System.Windows.Forms.Button
$installBtn.Text = "INSTALL"
$installBtn.Location = New-Object System.Drawing.Point(42, 460)
$installBtn.Size = New-Object System.Drawing.Size(180, 40)
$installBtn.FlatStyle = "Flat"
$installBtn.BackColor = $accent
$installBtn.ForeColor = $bg
$installBtn.Font = New-Object System.Drawing.Font("Segoe UI", 11, [System.Drawing.FontStyle]::Bold)
$form.Controls.Add($installBtn)

$closeBtn = New-Object System.Windows.Forms.Button
$closeBtn.Text = "CLOSE"
$closeBtn.Location = New-Object System.Drawing.Point(236, 460)
$closeBtn.Size = New-Object System.Drawing.Size(120, 40)
$closeBtn.FlatStyle = "Flat"
$closeBtn.BackColor = $panel
$closeBtn.ForeColor = $ink
$closeBtn.Add_Click({
    Remove-Log
    $form.Close()
})
$form.Controls.Add($closeBtn)
$form.Add_FormClosing({ Remove-Log })

function Test-Admin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($id)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Get-UserVstDir {
    return (Join-Path $env:LOCALAPPDATA "Programs\Common\VST3")
}

function Get-SystemVstDir {
    if ($env:CommonProgramFiles) { return (Join-Path $env:CommonProgramFiles "VST3") }
    return "C:\Program Files\Common Files\VST3"
}

function Remove-OldBundles([string]$dir) {
    if (-not (Test-Path $dir)) { return 0 }
    $n = 0
    foreach ($name in @("KYOTO.vst3", "KYOTRIPPAH FX.vst3", "KYOTRIPPAH.vst3", "KYOTRIPPAHFX.vst3")) {
        $path = Join-Path $dir $name
        if (-not (Test-Path -LiteralPath $path)) { continue }
        Add-Log "Removing old $path"
        try {
            Remove-Item -LiteralPath $path -Recurse -Force -ErrorAction Stop
            $n++
        } catch {
            Add-Log "Skipped $path ($($_.Exception.Message))"
        }
    }
    return $n
}

function Seat-SystemCopies {
    $system = Get-SystemVstDir
    $names = @("KYOTO.vst3", "KYOTRIPPAH FX.vst3")
    if (Test-Admin) {
        New-Item -ItemType Directory -Force -Path $system | Out-Null
        Remove-OldBundles $system | Out-Null
        foreach ($name in $names) {
            $src = Join-Path $LocalOut $name
            if (Test-Path -LiteralPath $src) { Copy-Bundle $src $system | Out-Null }
        }
        Add-Log "Seated system copies in $system"
        return $true
    }
    $helper = Join-Path $RunDir "installer\seat-system.ps1"
    $body = @"
`$ErrorActionPreference = 'Continue'
`$srcRoot = '$($LocalOut.Replace("'","''"))'
`$dest = '$($system.Replace("'","''"))'
New-Item -ItemType Directory -Force -Path `$dest | Out-Null
foreach (`$name in @('KYOTO.vst3','KYOTRIPPAH FX.vst3','KYOTRIPPAH.vst3','KYOTRIPPAHFX.vst3')) {
  `$old = Join-Path `$dest `$name
  if (Test-Path -LiteralPath `$old) { Remove-Item -LiteralPath `$old -Recurse -Force -ErrorAction SilentlyContinue }
}
foreach (`$name in @('KYOTO.vst3','KYOTRIPPAH FX.vst3')) {
  `$src = Join-Path `$srcRoot `$name
  if (Test-Path -LiteralPath `$src) { Copy-Item -LiteralPath `$src -Destination (Join-Path `$dest `$name) -Recurse -Force }
}
"@
    Set-Content -Path $helper -Value $body -Encoding UTF8
    Add-Log "Program Files is protected. Asking for admin to replace the old system copy."
    try {
        $p = Start-Process -FilePath "powershell.exe" -Verb RunAs -Wait -PassThru -ArgumentList "-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File `"$helper`""
        if ($p.ExitCode -eq 0 -and (Test-Path (Join-Path $system "KYOTO.vst3"))) {
            Add-Log "System copy replaced in $system"
            return $true
        }
        Add-Log "Admin step was cancelled or did not finish. User VST3 copy is still installed."
    } catch {
        Add-Log "Admin step skipped ($($_.Exception.Message)). User VST3 copy is still installed."
    }
    return $false
}

function Find-Cmake {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    foreach ($c in @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe")) {
        if (Test-Path $c) { return $c }
    }
    return $null
}

function Invoke-Hidden([string]$file, [string]$argList) {
    Add-Log "run $file $argList"
    $p = Start-Process -FilePath $file -ArgumentList $argList -WorkingDirectory $RunDir -WindowStyle Hidden -Wait -PassThru
    Add-Log "exit $($p.ExitCode)"
    return $p.ExitCode
}

function Copy-Bundle([string]$source, [string]$destDir) {
    New-Item -ItemType Directory -Force -Path $destDir | Out-Null
    $target = Join-Path $destDir $source.Split('\')[-1]
    # Name is the directory name
    $name = Split-Path -Leaf $source
    $target = Join-Path $destDir $name
    if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }
    Copy-Item -LiteralPath $source -Destination $target -Recurse -Force
    $so = Get-ChildItem -LiteralPath $target -Recurse -File | Select-Object -First 1
    if (-not $so) { throw "Copy of $name produced an empty bundle in $destDir" }
    Add-Log "Seated $name ($($so.Length) bytes) -> $target"
    return $target
}

function Install-FoundBundles([string[]]$roots) {
    $found = @()
    foreach ($root in $roots) {
        if (-not (Test-Path $root)) { continue }
        $found += Get-ChildItem -LiteralPath $root -Recurse -Directory -Filter "*.vst3" |
            Where-Object { $_.Name -match "KYOTO" }
    }
    $found = $found | Sort-Object FullName -Unique
    if (-not $found) { return 0 }
    $count = 0
    foreach ($bundle in $found) {
        Copy-Bundle $bundle.FullName $LocalOut | Out-Null
        try { Copy-Bundle $bundle.FullName (Get-UserVstDir) | Out-Null }
        catch { Add-Log "User VST3 copy failed ($($_.Exception.Message))" }
        $count++
    }
    return $count
}

$installBtn.Add_Click({
    $installBtn.Enabled = $false
    $script:installed = $false
    try {
        $bar.Value = 4
        $status.Text = "Working in $RunDir"
        Add-Log "Run folder: $RunDir"
        Add-Log "Project: $Project"
        if (-not (Test-Path (Join-Path $Project "CMakeLists.txt"))) {
            throw "Project\CMakeLists.txt is missing. Unzip the whole KYOTRIPPAH-0.5.0 folder, then open the installer from inside it."
        }

        $bar.Value = 12
        $status.Text = "Removing old versions"
        $removed = 0
        foreach ($dir in @((Get-UserVstDir), (Join-Path $env:USERPROFILE "Documents\VST3"), $LocalOut)) {
            $removed += Remove-OldBundles $dir
        }
        try { $removed += Remove-OldBundles (Get-SystemVstDir) } catch { Add-Log "System folder not writable yet." }
        Add-Log "Old bundles removed: $removed"

        $bar.Value = 22
        $cmake = Find-Cmake
        if (-not $cmake) {
            throw "CMake was not found. Install CMake and Visual Studio 2022 (Desktop development with C++), then run INSTALL again. Nothing was copied."
        }
        Add-Log "CMake: $cmake"
        if (Test-Path $BuildDir) {
            Add-Log "Clearing previous build folder"
            Remove-Item -LiteralPath $BuildDir -Recurse -Force -ErrorAction SilentlyContinue
        }
        New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
        $status.Text = "Downloading JUCE and configuring in this folder"
        $bar.Value = 35
        $code = Invoke-Hidden $cmake "-S `"$Project`" -B `"$BuildDir`" -G `"Visual Studio 17 2022`" -A x64"
        if ($code -ne 0) { throw "Configure failed (exit $code). Install the Visual Studio 2022 C++ workload. Build files are in $BuildDir" }

        $status.Text = "Building VST3 in this folder"
        $bar.Value = 60
        $code = Invoke-Hidden $cmake "--build `"$BuildDir`" --config Release --parallel"
        if ($code -ne 0) { throw "Build failed (exit $code). See this log. Nothing was installed." }

        $bar.Value = 82
        $status.Text = "Copying bundles into this folder and the VST3 folder"
        $n = Install-FoundBundles @($BuildDir)
        if ($n -lt 1) { throw "Build finished but no KYOTO .vst3 bundle was found under $BuildDir" }

        $check = Get-ChildItem -LiteralPath $LocalOut -Directory -Filter "*.vst3" -ErrorAction SilentlyContinue
        if (-not $check) { throw "Installed-VST3 is empty. Install did not land." }
        $userVst = Get-UserVstDir
        $seated = Get-ChildItem -LiteralPath $userVst -Directory -Filter "KYOTO.vst3" -ErrorAction SilentlyContinue
        if (-not $seated) { throw "KYOTO.vst3 is not in $userVst" }
        Seat-SystemCopies | Out-Null

        $bar.Value = 100
        $script:installed = $true
        $status.Text = "Installed. Rescan plugins."
        Add-Log "Local copy: $LocalOut"
        Add-Log "DAW folder: $userVst"
        Add-Log "Log saved to $LogFile until you hit CLOSE."
        Save-Log
        [System.Windows.Forms.MessageBox]::Show(
            "KYOTRIPPAH 0.5.1 is installed.`r`n`r`nThis folder:`r`n$LocalOut`r`n`r`nVST3 folder:`r`n$userVst`r`n`r`nRescan plugins. CLOSE deletes the Downloads log.",
            "Installed",
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Information) | Out-Null
    } catch {
        $status.Text = "Install failed"
        Add-Log ("FAILED: " + $_.Exception.Message)
        Save-Log
        Add-Log "Log saved to $LogFile until you hit CLOSE."
        [System.Windows.Forms.MessageBox]::Show(
            $_.Exception.Message + "`r`n`r`nLog: $LogFile",
            "Install failed",
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Error) | Out-Null
    } finally {
        $installBtn.Enabled = $true
    }
})

Add-Log "Ready. INSTALL builds inside this folder, deletes old KYOTRIPPAH VST3s, and copies the new ones."
Add-Log "A log is written to Downloads after INSTALL. CLOSE deletes it."
[void]$form.ShowDialog()
