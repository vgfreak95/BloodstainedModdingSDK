[![CI](https://github.com/vgfreak95/BloodstainedModdingSDK/actions/workflows/msbuild.yml/badge.svg)](https://github.com/vgfreak95/BloodstainedModdingSDK/actions/workflows/msbuild.yml)
[![License](https://img.shields.io/github/license/vgfreak95/BloodstainedModdingSDK)](LICENSE)
[![Last Commit](https://img.shields.io/github/last-commit/vgfreak95/BloodstainedModdingSDK)](https://github.com/vgfreak95/BloodstainedModdingSDK/commits/main)

# BloodstainedModdingSDK
The BloodstainedModdingSDK is an SDK (Source Development Kit) which allows Developers to start Modding Bloodstained.
In the future, the project will be forked, and converted into an Archipelago. Given all tests are run on Steam version, there are no guarantees
the SDK will function on other platforms.

## Installation:
1. Go to Releases, and grab the latest `.dll` file and drag into your games Shipping directory. On Steam that's `steamapps\common\Bloodstained Ritual of the Night\BloodstainedRotN\Binaries\Win64`
2. Launch the Game, and press F2 to open the ImGui window in game. Explore the Gui to your hearts content and add more custom inside `Mod\Gui.cpp`.

## Building from Source:

### Prerequisites:
- Visual Studio version that supports .slnx
- MSBuild tools v143

### Building:
1. Clone/Download the `main` branch of the repository with command `git clone --recurse-submodules=subprojects`. `main` will always be stable latest functional code.
2. Open a VS Administrator Terminal, and `cd` into the project root directory/folder.
3. Run `vcpkg install --triplet x64-windows-static-md` to install openssl and zlib. The packages are defined in `vcpkg.json` in project root directory/folder.
4. Open the `BloodstainedModdingSDK.vcxproj` and modify the `<BSGamePath>` sections to match your Games target destination.
5. There are 2 Configurations available (Release WIP), use Debug x64 (should be default), then in Visual Studio, at the top Build -> Build Solution.
6. If there are any errors check FAQ section (WIP).
7. Check the `BSGamePath` location, and ensure `version.dll` was populated.
8. Launch the Game, and press F2 to open the ImGui window in game. Explore the Gui to your hearts content and add more custom inside Mod\Gui.cpp.

## Contributing:
Fork the main repository, and make a PR. I haven't made a thorough enough system, and don't believe it will get to that point.

## Shoutouts:
- Trexounay: For building EnderMagnolia Randomizer, its architecture is used as a foundation for this project.
- Lakifume: For building the Bloodstained TrueRandomizer, will be a huge help for research
- Tourmi: For creating an Baseline APWorld, various modding repositories and attempting this themselves. Their Archipelago foundation will help jumpstart the Archipelago portion of this project.
