# Unreleased

# Added

- Support for HDMI/DisplayPort input and output

# Changed

- Handling of dependencies with Conan and submodules
- Handling of VideoMaster API, now using the official C++ API
- Application name is now `videomaster-overlay-from-live-content`

# 1.0.2

## Added

- Parameter for handling maximum desired input to output latency (default is 2)

# 1.0.1

## Fixed

- Compilation on Windows
- Suitability of device not implemented
- Documentation:
  - Missing information about VideoMaster SDK environment variables
  - Minimum latency should be 2 and not 3
  - Some typos


# 1.0.0

## Added

- Application that is capable of
  - Receiving video from an input
  - Generating some content from that input
  - Sending the result to an output (either through the on-board keyer or not)
- Support of Windows and Linux
- Support of SDI and keyer-enable devices
- Support of overlay and non-overlay processing
- Support of rendering the live content on the screen
- Detection of change in the input format and automatic reconfiguration of the output
