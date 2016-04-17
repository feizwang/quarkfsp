# QuarkFsp

Source code access:

1. Go to https://review.gerrithub.io/#/admin/projects/feizwang/quarkfsp
2. Click on "Clone with commit-msg hook" tab
3. Click on ssh tab
4. Copy the git clone string

Pushing patches:

1. Make sure patch includes: "Contributed-under: TianoCore Contribution
Agreement 1.0"
2. git push origin HEAD:refs/for/master
3. Review patches with [Gerrit](https://review.gerrithub.io/)
4. Add reviewers: Fei Wang, Lee Leahy

Work List:

* Get things to build with Linux
* Move BIOS code from Platform/Pei/PlatformInit/BiosCode* into coreboot and
TianoCore
* Reduce to minimal FSP
* Pass in memory data via SPDs
* Convert to FSP 2.0

# Packages

* [QuarkFspPkg](QuarkFspPkg)
