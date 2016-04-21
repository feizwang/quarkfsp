# EDK II firmware of Intel(R) Firmware Support Package (Intel(R) FSP) for Intel(R) Quark SoC X1000

## Work List

* Move BIOS code from Platform/Pei/PlatformInit/BiosCode* into coreboot and
TianoCore
* Reduce to minimal FSP
* Pass in memory data via SPDs
* Convert to FSP 2.0

## Get the Source

```
git clone https://github.com/tianocore/edk2.git
git clone https://github.com/tianocore/edk2-non-osi.git
cd edk2
git clone ssh://LeeLeahy@review.gerrithub.io:29418/LeeLeahy/quarkfsp && scp -p -P 29418 LeeLeahy@review.gerrithub.io:hooks/commit-msg quarkfsp/.git/hooks/
mv quarkfsp QuarkFspPkg
```

HTTPS is also available:
1. Go to https://review.gerrithub.io/#/admin/projects/LeeLeahy/quarkfsp
2. Click on "Clone with commit-msg hook" tab
3. Click on http tab
4. Copy the git clone string
5. Rename the directory from quarkfsp to QuarkFspPkg

## Pushing patches

1. Make sure patch includes: "Contributed-under: TianoCore Contribution
Agreement 1.0"
2. git push origin HEAD:refs/for/master
3. Review patches with [Gerrit](https://review.gerrithub.io/)
4. Add reviewers: Fei Wang, Lee Leahy

## **Linux Build Instructions**

### Setup the build environment

```
export PACKAGES_PATH=$PWD/edk2:$PWD/edk2-non-osi
cd edk2
export WORKSPACE=$PWD
make -C BaseTools
. edksetup.sh
```

### Build FSP

```
QuarkFspPkg/BuildFsp.sh -d32
```

Delete the generated files:
```
QuarkFspPkg/BuildFsp.bat -clean
```

## **Windows Build Instructions**

### Pre-requisites

* GIT client: Available from https://git-scm.com/downloads
* Python 2.6/2.7: Available from https://www.python.org
* Microsoft Visual Studio.
  - Visual Studio 2015 recommended and is used in the examples below.
* Microsoft Windows Driver Development Kit 3790.1830
  - http://download.microsoft.com/download/9/0/f/90f019ac-8243-48d3-91cf-81fc4093ecfd/1830_usa_ddk.iso
  - Mount ISO image
  - Right click on ```x86\kitsetup.exe``` & choose **Run as administrator**
  - Install to C:\WINDDK\3790.1830
  - Uncheck all Component Groups
  - Expand Build Environment Component
  - Check Windows Driver Development Kit 16-bit Additional Build Tools
  - Install

Create a new directory for an EDK II WORKSPACE.

The code block below shows the GIT clone operations required to pull the EDK II
source tree, the QuarkFspPkg sources, the pre-built versions of BaseTools as WIN32
binaries, and the edk2-non-osi repository that provides a binary file for the
Quark Remote Management Unit (RMU).

```cmd
git clone https://github.com/tianocore/edk2.git
git clone https://github.com/tianocore/edk2-BaseTools-win32.git
git clone https://github.com/tianocore/edk2-non-osi.git

set WORKSPACE=%CD%
set PACKAGES_PATH=%WORKSPACE%\edk2;%WORKSPACE%\edk2-non-osi
set EDK_TOOLS_BIN=%WORKSPACE%\edk2-BaseTools-win32

QuarkFspPkg/BuildFsp.bat /d32
```

Delete the generated files:

```
QuarkFspPkg/BuildFsp.bat /clean
```
