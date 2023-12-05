# QCoro Release Checklist

1. Ensure all CI builds for current `main` pass
1. Create Release Announcement in `docs/news/`
1. Update `docs/changelog.md`
1. Bump version in `qcoro_VERSION` in CMakeLists.txt to `X.Y.Z`
1. Commit release announcement and version bump as `QCoro release X.Y.Z`
1. Tag the release commit: `git tag -s vX.Y.Z -m "QCoro X.Y.Z"`
1. Bump version in `qcoro_VERSION` in CMakeLists.txt to `X.Y.80`
1. Commit version bump as `Bump version to X.Y.80 (X.(Y+1).0 development)`
1. Push tag and commits: `git push --tags origin main`
1. [Create release in Github](https://github.com/danvratil/qcoro/releases/new)
1. Submit PR to update QCoro in [ConanCenter](https://github.com/conan-io/conan-center-index/)
1. Submit PRs to update QCoro in [vcpkg](https://github.com/microsoft/vcpkg)
1. Copy the release announcement to my blog to publicize the release on Planet KDE
1. Post an announcement to Twitter
1. Add any extra step taken during last release and not included in this list
