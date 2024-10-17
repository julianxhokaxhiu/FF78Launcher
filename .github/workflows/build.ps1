#*****************************************************************************#
#    Copyright (C) 2024 Julian Xhokaxhiu                                      #
#                                                                             #
#    This file is part of FF78Launcher                                                #
#                                                                             #
#    FF78Launcher is free software: you can redistribute it and\or modify             #
#    it under the terms of the GNU General Public License as published by     #
#    the Free Software Foundation, either version 3 of the License            #
#                                                                             #
#    FF78Launcher is distributed in the hope that it will be useful,                  #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of           #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            #
#    GNU General Public License for more details.                             #
#*****************************************************************************#

Set-StrictMode -Version Latest

if ($env:_BUILD_BRANCH -eq "refs/heads/master" -Or $env:_BUILD_BRANCH -eq "refs/tags/canary")
{
  $env:_IS_BUILD_CANARY = "true"
  $env:_IS_GITHUB_RELEASE = "true"
}
elseif ($env:_BUILD_BRANCH -like "refs/tags/*")
{
  $env:_CHANGELOG_VERSION = $env:_BUILD_VERSION.Substring(0,$env:_BUILD_VERSION.LastIndexOf('.')).Replace('.','')
  $env:_BUILD_VERSION = $env:_BUILD_VERSION.Substring(0,$env:_BUILD_VERSION.LastIndexOf('.')) + ".0"
  $env:_IS_GITHUB_RELEASE = "true"
}
$env:_RELEASE_VERSION = "v${env:_BUILD_VERSION}"

$vcpkgRoot = "C:\vcpkg"
$vcpkgBaseline = [string](jq --arg baseline "builtin-baseline" -r '.[$baseline]' vcpkg.json)
$vcpkgOriginUrl = &"git" -C $vcpkgRoot remote get-url origin
$vcpkgBranchName = &"git" -C $vcpkgRoot branch --show-current

$releasePath = [string](jq -r '.configurePresets[0].binaryDir' CMakePresets.json).Replace('${sourceDir}/', '')

Write-Output "--------------------------------------------------"
Write-Output "BUILD CONFIGURATION: $env:_RELEASE_CONFIGURATION"
Write-Output "RELEASE VERSION: $env:_RELEASE_VERSION"
Write-Output "VCPKG ORIGIN: $vcpkgOriginUrl"
Write-Output "VCPKG BRANCH: $vcpkgBranchName"
Write-Output "VCPKG BASELINE: $vcpkgBaseline"
Write-Output "--------------------------------------------------"

Write-Output "_BUILD_VERSION=${env:_BUILD_VERSION}" >> ${env:GITHUB_ENV}
Write-Output "_RELEASE_VERSION=${env:_RELEASE_VERSION}" >> ${env:GITHUB_ENV}
Write-Output "_IS_BUILD_CANARY=${env:_IS_BUILD_CANARY}" >> ${env:GITHUB_ENV}
Write-Output "_IS_GITHUB_RELEASE=${env:_IS_GITHUB_RELEASE}" >> ${env:GITHUB_ENV}
Write-Output "_CHANGELOG_VERSION=${env:_CHANGELOG_VERSION}" >> ${env:GITHUB_ENV}

# Load vcvarsall environment for x86
$vcvarspath = &"${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -prerelease -latest -property InstallationPath
cmd.exe /c "call `"$vcvarspath\VC\Auxiliary\Build\vcvarsall.bat`" x86 && set > %temp%\vcvars.txt"
Get-Content "$env:temp\vcvars.txt" | Foreach-Object {
  if ($_ -match "^(.*?)=(.*)$") {
    Set-Content "env:\$($matches[1])" $matches[2]
  }
}

# Unset VCPKG_ROOT if set
[Environment]::SetEnvironmentVariable('VCPKG_ROOT','')

# Add Github Packages registry
nuget sources add -Name github -Source "https://nuget.pkg.github.com/julianxhokaxhiu/index.json" -Username ${env:GITHUB_REPOSITORY_OWNER} -Password ${env:GITHUB_PACKAGES_PAT} -StorePasswordInClearText
nuget setApiKey ${env:GITHUB_PACKAGES_PAT} -Source "https://nuget.pkg.github.com/julianxhokaxhiu/index.json"
nuget sources list

# Vcpkg setup
git -C $vcpkgRoot pull --all
git -C $vcpkgRoot checkout $vcpkgBaseline
git -C $vcpkgRoot clean -fxd

cmd.exe /c "call $vcpkgRoot\bootstrap-vcpkg.bat"

vcpkg integrate install

# Start the build
cmake --preset "${env:_RELEASE_CONFIGURATION}" -D_EXE_VERSION="$env:_BUILD_VERSION"
cmake --build --preset "${env:_RELEASE_CONFIGURATION}"

# Start the packaging
mkdir .dist\pkg | Out-Null
Copy-Item -R "$releasePath\bin\COPYING.TXT" ".dist\pkg\"
Copy-Item -R "$releasePath\bin\FF78Launcher.toml" ".dist\pkg\"
Copy-Item -R "$releasePath\bin\FF78Launcher.exe" ".dist\pkg\FF7_Launcher.exe"
Copy-Item -R "$releasePath\bin\FF78Launcher.exe" ".dist\pkg\FF8_Launcher.exe"
7z a ".\.dist\${env:_RELEASE_NAME}-${env:_RELEASE_VERSION}.zip" ".\.dist\pkg\*"

Remove-Item -Recurse -Force .dist\pkg
