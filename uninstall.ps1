# VLC Discord RPC Plugin Uninstaller
# This script safely removes the [discord_rpc] configuration section, disables the plugin, 
# and deletes the plugin DLL using registry-based path discovery.

# --- PRE-EXECUTION CHECKS ---

# 1. Check for Administrator privileges
$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "ERROR: This script must be run as Administrator to remove the plugin DLL." -ForegroundColor Red
    Write-Host "Please right-click the script and select 'Run with PowerShell' after opening PowerShell as Admin."
    exit 1
}

# 2. Check if VLC is running
if (Get-Process "vlc" -ErrorAction SilentlyContinue) {
    Write-Host "ERROR: VLC is currently running." -ForegroundColor Red
    Write-Host "Please close VLC before proceeding with the uninstallation."
    exit 1
}

# --- VLC CONFIGURATION CLEANUP ---

# Use portable environment variable for AppData
$vlcrcPath = Join-Path $env:APPDATA "vlc\vlcrc"

# SECURITY CHECK: Ensure the file exists before proceeding
if (-not (Test-Path $vlcrcPath)) {
    Write-Host "WARNING: VLC configuration file (vlcrc) not found. Skipping config cleanup." -ForegroundColor Yellow
}
else {
    Write-Host "Modifying VLC configuration: $vlcrcPath"

    # SAFEGUARD: Create an automatic backup to prevent data loss
    $backupPath = "$vlcrcPath.bak"
    try {
        Copy-Item $vlcrcPath $backupPath -Force -ErrorAction Stop
        Write-Host "Backup created: $backupPath"
    }
    catch {
        Write-Host "ERROR: Could not create backup. Aborting config cleanup." -ForegroundColor Red
        # We continue to DLL removal if possible
    }

    if (Test-Path $backupPath) {
        try {
            # Read the entire file content
            $content = Get-Content -Raw $vlcrcPath

            # 1. REMOVE [discord_rpc] SECTION
            $sectionHeader = "[discord_rpc]"
            $startIndex = $content.IndexOf($sectionHeader)
            
            if ($startIndex -ne -1) {
                $nextSectionIndex = $content.IndexOf("[", $startIndex + $sectionHeader.Length)
                
                if ($nextSectionIndex -eq -1) {
                    $content = $content.Substring(0, $startIndex)
                }
                else {
                    $content = $content.Substring(0, $startIndex) + $content.Substring($nextSectionIndex)
                }
                Write-Host "Removed [discord_rpc] configuration section."
            }

            # 2. DEACTIVATE PLUGIN FROM 'control=' LINE
            if ($content -match '(?m)^control=.*') {
                $originalControlLine = $Matches[0]
                
                $newControlLine = $originalControlLine -replace 'discord_rpc', ''
                $newControlLine = $newControlLine -replace ',+', ','
                $newControlLine = $newControlLine -replace '=,', '='
                $newControlLine = $newControlLine -replace ',$', ''
                
                if ($originalControlLine -ne $newControlLine) {
                    $content = $content -replace [regex]::Escape($originalControlLine), $newControlLine
                    Write-Host "Plugin removed from VLC control interface list."
                }
            }

            # 3. SAVE CHANGES
            $content | Set-Content $vlcrcPath -NoNewline -ErrorAction Stop
            Write-Host "VLC configuration cleanup successful." -ForegroundColor Green

        }
        catch {
            Write-Host "CRITICAL ERROR: Failed to modify vlcrc: $($_.Exception.Message)" -ForegroundColor Red
            Write-Host "Attempting to restore from backup..." -ForegroundColor Yellow
            Copy-Item $backupPath $vlcrcPath -Force
        }
    }
}

# --- PLUGIN DLL REMOVAL ---

Write-Host "`nSearching for VLC installation path..."

# Look for VLC installation path in the registry
$regPath = "HKLM:\SOFTWARE\VideoLAN\VLC"
$vlcInstallDir = $null

if (Test-Path $regPath) {
    $vlcInstallDir = Get-ItemProperty -Path $regPath -Name "InstallDir" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty InstallDir
}

# If registry fails, check common 64-bit and 32-bit paths
if (-not $vlcInstallDir) {
    $vlcInstallDir = "C:\Program Files\VideoLAN\VLC"
}

if ($vlcInstallDir) {
    $dllPath = Join-Path $vlcInstallDir "plugins\misc\libdiscordrpc_plugin.dll"
    $vlcExePath = Join-Path $vlcInstallDir "vlc.exe"

    if (Test-Path $dllPath) {
        try {
            Remove-Item $dllPath -Force -ErrorAction Stop
            Write-Host "Successfully deleted: $dllPath" -ForegroundColor Green

            # Reset VLC plugins cache
            if (Test-Path $vlcExePath) {
                Write-Host "Refreshing VLC plugin cache..."
                Start-Process -FilePath $vlcExePath -ArgumentList "--reset-plugins-cache", "vlc://quit" -Wait -NoNewWindow
                Write-Host "VLC plugin cache updated."
            }
        }
        catch {
            Write-Host "ERROR: Could not delete DLL file. Please check permissions." -ForegroundColor Red
        }
    }
    else {
        Write-Host "Plugin DLL not found at: $dllPath" -ForegroundColor Yellow
    }
}
else {
    Write-Host "ERROR: VLC installation directory not found. Manual DLL removal might be required." -ForegroundColor Red
}

Write-Host "`nUninstallation process finished." -ForegroundColor Cyan
