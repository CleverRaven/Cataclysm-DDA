# Disclaimer

**WARNING**: Xcode build should be used for _iOS builds only_. This can only be done on OS X.

To build CataclysmDDA for other OS, see:

-   [COMPILING.md](COMPILING.md)

# Prerequisites

The project will build for tiles and sound by default.

_Note: At this time there is no specific guidance on minimum versioning, all testing has been done on iOS 15.0+_

### Get the tools

-   Xcode on the AppStore - https://apps.apple.com/us/app/xcode/id497799835.
-   Install Brew - https://brew.sh/
    Or in a terminal window run:
    `/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"`

### Build SDL libs for iOS

You will need to download the SDL source code and build static libraries for iOS (ARM64). The source code is available [here](https://github.com/libsdl-org/SDL). Directions on building the SDL for iOS in Xcode is available [here](https://github.com/libsdl-org/SDL/blob/main/docs/README-ios.md). The project setup will expect to find the static libraries in `/build/libs` and related header files in `/build/libs/headers`.

### (Optional) Apple Developer ID pre-configuration

If you already know you developer ID, you can add it to the file `xcode_dev_id_example.yml`, and rename the file to `xcode_dev_id.yml`.

Additional details are in the example file.

### Run the script

Open terminal window to the build-scripts directory and run:
`./ios-xcode.sh`

If the Xcode project builds successfully Xcode will try to open automatically.

### Xcode Steps

If you have not already set the developer ID you will need to set it in the Project settings in the section "Signing & Capabilities"

Product->Run will build for debugging
Product->Archive will build a release version.

### Play!

Configuration and save files are accessible in your documents folder via the Files app. This should allow you to import and export data from other places. For now there is no auto-synchronization with any cloud based data services.

### Notes

I appreciate the many folks in the community who have contributed to making such a wonderful game, and my intent was to make sure that everyone had access to an iPad version that was freely available and built to integrate with the existing codebase, making it inclusive of experimental builds.

If there are other setup related steps needed, please update accordingly or find me on Discord with questions or issues (smileynet on both Official and Community servers) understanding that my support of this build is at-will and may be intermittent. (Written on 10/26/2022, if you are unable to @ me on either server, I have likely wandered on down the road or been ill prepared for an encounter with a Kevlar Hulk o.O)

My use case for this build is playing on an iPad with an attached magic keyboard, so while I've tested the soft keyboard's basic functionality, at this point I have not done any of the UI enhancements found in the other ports found on the AppStore (or the Android build, but with any luck that will be reasonable to reuse in this case).