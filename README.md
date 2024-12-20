# Overlay From Live Content

The application demonstrates an overlay use case built on any keyer-enable DELTACAST device where the overlay is computed from the live content fed on one of its input connectors.

An SDI or DV keyer-enable device is currently the only valid type of devices for this application.

The actual processing that is implemented in the demo application is quite straightforward.
In this case, it consists in overlaying the bottom half of the input frame onto the live content so that the top half is the live content while the bottom half is delayed by the processing.
That way, we can see the live content and the delayed content at the same time and compare them in terms of latency.

OS Support:
- Windows
- Linux

See https://www.deltacast.tv for more video products.

# How to build

As some dependencies are retrieved through submodules, you will need to initialize them:

    git submodule update --init --recursive

VideoViewer requires some dependencies to be installed on the system:

    cmake v3.20 or higher
    glfw v3.4.0
    Python 3

We recommend using Conan 2.x to retrieve those dependencies:

    conan install . -b missing -pr YOUR_CONAN_PROFILE

## VideoMaster SDK

The VideoMaster SDK (version >= 6.26) is required to build the application.

After installing the SDK according to the official documentation, the libs and headers should be found without further step needed through the `find_package` command.

# Building with CMake

If you used Conan to retrieve your dependencies, you can use the following commands to build the project:

    cmake --preset YOUR_CMAKE_PRESET
    cmake --build build

# How to use

All relevant information regarding the application can be found by running the application with the `--help` option:

```shell
./videomaster-overlay-from-live-content --help
```

For example, to run the application with the default settings, simply run:

```shell
./videomaster-overlay-from-live-content
```

Activating the rendering of the live content on the screen and the handling of the keyer can be done with the following command:

```shell
./videomaster-overlay-from-live-content --renderer --overlay
```

## How to customize

The application is designed to be easily customizable in terms of processing and memory allocation of the buffers.

`processing.cpp` contains code for overlay and non-overlay processing which can be modified to implement any kind of processing.
Pay extra care that the processing time shall be less than the time between two frames, otherwise the application will not be able to keep up with the incoming frames and will constantly drop content.

`allocation.cpp` contains code for buffer allocation which can be modified to implement any kind of buffer allocation, be it on the GPU or the host memory.

# Some technical explanations

Some technical explanations are available in the following [page](technical_details.md).