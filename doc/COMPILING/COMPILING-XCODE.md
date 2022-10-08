# Disclaimer

**WARNING**: Xcode build is **NOT** official and should be used for *iOS builds only*. This can only be done on OS X.

For the official way to build CataclysmDDA, see:
  * [COMPILING.md](COMPILING.md)

# Prerequisites

Currently all required libraries are either pre-installed on the OS or privded as static libraries as part of the project. The project will build for tiles and sound by default.

*TODO: need to confirm minimum version targets*

### Get the tools

* Xcode on the AppStore - https://apps.apple.com/us/app/xcode/id497799835.
* Install Brew - https://brew.sh/
    Or in a terminal window run:
    `/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"`

### (Optional) Apple Developer ID pre-configuration

If you already know you developed ID, you can add it to the file `xcode_dev_id_example.yml`, and rename the file to `xcode_dev_id.yml`.

Additional details can be found inside the example file.

### Run the script

Open terminal window to the build-scripts directory and run:
`./ios-xcode.sh`

If the Xcode project builds succesfully Xcode should open automatically.

### Xcode Steps

If you did not previously set the developer ID you will need to set it in the Project settings in the section "Signing & Capabilities"

Currently on live targets (not simulators) are supported.

Product->Run will build for debugging
Product->Archive will build a release version.

### Play!

Configuration and save files are accessible in your documents folder via the Files app. This should allow you to import and export data from other place. For now there is no auto-synchronization with any cloud based data services.

### Notes

I appreciate the many folks in the community who have contributed to making such a wonderful game, and my intent was to make sure that everyone had access to an ipad version that was freely available and built to integrate with the existing codebase.

There are likely other first time developer setup releated steps that may need to be captured here, please update accoringly or find me on Discord with questions or issues (smileynet#7052) understanding that my support of this build is at-will and may be intermittent.

My use case for this build is playing on an iPad with an attached magic keyboard, so while I've tested the soft keyboard's basic functionality, at this point I have not done any of the UI enhancements found in the other ports found on the AppStore (or the Android build, but I'm looking at adapting that since it is likely to be easier to implement).