# hello-ospray

## Description

This is a "hello world" example for the Intel OSPRay ray-tracing engine v1.8. In this example, a textured triangle is rendered and saved to a file. In order to make resource management more explicit in this example, the C language API of OSPRay is used.

This example shows:
- How to initialise OSPRay.
- How to create OSPRay geometry consisting of a vertex buffer, index buffer and UV buffer.
- How to create an OSPRay texture material and bind these to some OSPRay geometry.
- How to invoke the renderer and map the rendered image so it can be saved to file.
- How to correctly clean up OSPRay resources.

## Build Instructions

Create a build output folder and then navigate to that folder. Simply run `cmake /path/to/hello-ospray` followed by `make`. OSPRay 
version 1.8.x must be installed and locatable by CMake.

## Run Instructions

The binary `hello-ospray`will be put into the build output folder. Running this program will produce an image called output.png.
