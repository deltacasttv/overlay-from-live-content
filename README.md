# Overlay From Live Content

The application demonstrates an overlay use case built on any keyer-enable DELTACAST device where the overlay is computed from the live content fed on one of its input connectors.

An SDI and keyer-enable device is currently the only valid type of device for this application.

The actual processing that is implemented in the demo application is quite straightforward.
In this case, it consists in overlaying the bottom half of the input frame onto the live content so that the top half is the live content while the bottom half is delayed by the processing.
That way, we can see the live content and the delayed content at the same time and compare them in terms of latency.

OS Support:
- Windows
- Linux

See https://www.deltacast.tv for more video products.

## How to build

VideoViewer requires some dependencies to be installed on the system:
- cmake v3.19 or higher
- glfw v3.3.6
- Python 3

### Retrieve dependencies with Conan (optional)

To use Conan 1.x to retrieve the dependencies, create the `modules`` directory and use the install command:

```shell
mkdir /path/to/modules
cd /path/to/modules
conan install /path/to/video-viewer -b missing -g cmake_find_package
```

### VideoMaster SDK

The VideoMaster SDK is required to build the application.

#### Windows

After installing the SDK according to the official documentation, you shall place the `Include` and `Library` folders in a new folder `deps/VideoMaster`.

#### Linux

After installing the SDK according to the official documentation, the libs and headers should be found without further steps needed.

### Building with CMake

If you used Conan to retrieve your dependencies, you can use the following commands to build the project:

```shell
cd /path/to/video-viewer
cmake -S . -B build -DCMAKE_MODULE_PATH:PATH=/path/to/modules
cmake --build build
```

## How to use

All relevant information regarding the application can be found by running the application with the `--help` option:

```shell
./OverlayFromLiveContent --help
```

For example, to run the application with the default settings, simply run:

```shell
./OverlayFromLiveContent
```

Activating the rendering of the live content on the screen and the handling of the keyer can be done with the following command:

```shell
./OverlayFromLiveContent --renderer --overlay
```

## How to customize

The application is designed to be easily customizable in terms of processing and memory allocation of the buffers.

`processing.cpp` contains code for overlay and non-overlay processing which can be modified to implement any kind of processing.
Note that a predictable and stable processing time is preferrable to avoid any frame drops.
Indeed, since the application constantly tries to minimize the latency, oscillations in the processing time will result in oscillations in the latency which can materialize as frame drops.

`allocation.cpp` contains code for buffer allocation which can be modified to implement any kind of buffer allocation, be it on the GPU or the host memory.

# Some technical explanations

Some technical explanations are available in the following [page](technical_details.md).