#include <cstdint>
#include <cstdlib>

#include <iostream>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "ospray/ospray.h"

int main(int argc, char **argv) {
  if (ospInit(&argc, (const char **) argv) != OSP_NO_ERROR) {
    std::cerr << "An error occurred while attempting to initialize OSPRay " <<
      "so this program will now exit." << std::endl;
    return EXIT_FAILURE;
  }

  // Create a triangle. A triangle is defined by three vertices (points in
  // world space) and an index buffer. The index buffer is a compression
  // technique for large meshes. It isn't useful for a single triangle but is
  // required by OSPRay nevertheless.
  OSPGeometry triangle = ospNewGeometry("triangles");

  // The triangle's vertices are specified in world space. To keep things
  // simple, the vertices are specified near the origin where the camera will
  // be pointing by default. Winding isn't important here.
  const float TRIANGLE_VERTICES[] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f
  };
  const int32_t TRIANGLE_INDICES[] = {
    0, 1, 2
  };
  const float TRIANGLE_UVS[] = {
    0.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 0.0f
  };
  // OSPRay must be given the necessary data to be able to render the triangle,
  // but OSP_DATA_SHARED_BUFFER can be used to avoid copying. However, this puts
  // responsibility on the application to keep this data available while OSPRay
  // is still using it.
  OSPData triangleVertexBuffer = ospNewData(
    sizeof(TRIANGLE_VERTICES) / sizeof(TRIANGLE_VERTICES[0]),
    OSP_FLOAT3,
    TRIANGLE_VERTICES,
    OSP_DATA_SHARED_BUFFER);
  ospCommit(triangleVertexBuffer);
  OSPData triangleIndexBuffer = ospNewData(
    sizeof(TRIANGLE_INDICES) / sizeof(TRIANGLE_INDICES[0]),
    OSP_INT,
    TRIANGLE_INDICES,
    OSP_DATA_SHARED_BUFFER);
  ospCommit(triangleIndexBuffer);
  OSPData triangleUVbuffer = ospNewData(
    sizeof(TRIANGLE_UVS) / sizeof(TRIANGLE_UVS[0]) / 2,
    OSP_FLOAT2,
    TRIANGLE_UVS,
    OSP_DATA_SHARED_BUFFER);
  ospCommit(triangleUVbuffer);
  ospSetData(triangle, "vertex", triangleVertexBuffer);
  ospSetData(triangle, "index", triangleIndexBuffer);
  ospSetData(triangle, "vertex.texcoord", triangleUVbuffer);
  // OSPRay internally maintains reference counts for OSPData. The vertex and
  // index buffers will each have a reference count of 2: one from ospNewData
  // and one from ospSetData. Pass ownership of the buffers to OSPRay by
  // releasing the application's references.
  ospRelease(triangleVertexBuffer);
  ospRelease(triangleIndexBuffer);
  ospRelease(triangleUVbuffer);

  // Create a texture for the triangle.
  OSPTexture texture = ospNewTexture("texture2d");
  const uint8_t TEXELS[] = {
    255,   0,   0, // Red
      0, 255,   0, // Green
      0,   0, 255, // Blue
    255, 255,   0, // Yellow
  };
  ospSet2i(texture, "size", 2, 2);
  ospSet1i(texture, "type", OSP_TEXTURE_RGB8);
  // Using this filter, the texture will have a blocky appearance. For this
  // example, it's useful to see how the colours are mapped to the UV
  // coordinates of the triangle's vertices. Besides, the low-resolution texture
  // won't look good if the alternative (bilinear interpolation) is used.
  ospSet1i(texture, "flags", OSP_TEXTURE_FILTER_NEAREST); 
  OSPData texelsBuffer = ospNewData(
    sizeof(TEXELS) / sizeof(TEXELS[0]),
    OSP_UCHAR,
    TEXELS,
    OSP_DATA_SHARED_BUFFER);
  ospCommit(texelsBuffer);
  ospSetData(texture, "data", texelsBuffer);
  ospCommit(texture);
  ospRelease(texelsBuffer);

  // When using the path tracer, geometries will appear black unless they have
  // a material.
  OSPMaterial material = ospNewMaterial2("pathtracer", "OBJMaterial");
  // Texturing is achieved by binding the texture to the diffuse colour channel
  // of the material.
  ospSetObject(material, "map_Kd", texture);
  ospCommit(material);
  ospRelease(texture);
  ospSetMaterial(triangle, material);
  ospCommit(triangle);
  ospRelease(material);

  OSPModel scene = ospNewModel();
  ospAddGeometry(scene, triangle);
  ospCommit(scene);
  ospRelease(triangle);

  // Use an orthographic projection with a view volume that fits the triangle.
  // The camera will be positioned in -Z pointing towards the origin, where the
  // triangle is located.
  OSPCamera camera = ospNewCamera("orthographic");
  ospSet1f(camera, "height", 2.0f);
  ospSet1f(camera, "width", 2.0f);
  ospSet3f(camera, "pos", 0.0f, 0.0f, -1.0f);
  ospCommit(camera);

  // Without illumination no colours will be visible, even if the triangle is
  // textured.
  std::vector<OSPLight> lights;
  lights.push_back(ospNewLight3("ambient"));
  ospCommit(lights[0]);
  OSPData lightData = ospNewData(lights.size(), OSP_LIGHT, lights.data(), 0);
  ospCommit(lightData);

  // Create a renderer. Since the lighting is very simple, a low
  // samples-per-pixel can be used.
  OSPRenderer renderer = ospNewRenderer("pathtracer");
  ospSetObject(renderer, "model", scene);
  ospSetObject(renderer, "camera", camera);
  ospSetObject(renderer, "lights", lightData);
  ospSet1i(renderer, "spp", 1);
  ospCommit(renderer);
  ospRelease(camera);
  ospRelease(scene);
  ospRelease(lightData);

  // Render an image and save it to file.
  const osp::vec2i IMAGE_DIMENSIONS = {.x = 400, .y = 400};
  OSPFrameBuffer frameBuffer = ospNewFrameBuffer(IMAGE_DIMENSIONS, OSP_FB_RGBA8,
    OSP_FB_COLOR);
  ospCommit(frameBuffer);

  ospRenderFrame(frameBuffer, renderer, OSP_FB_COLOR);

  // Note that the origin of image space in OSPRay is at the bottom-left of the
  // image. When simply copying the pixels to a file, the origin will be at the
  // top-left of the image.
  const void *pixels = ospMapFrameBuffer(frameBuffer, OSP_FB_COLOR);
  stbi_write_png("output.png", IMAGE_DIMENSIONS.x, IMAGE_DIMENSIONS.y, 4,
    pixels, IMAGE_DIMENSIONS.x * 4);
  ospUnmapFrameBuffer(pixels, frameBuffer);

  // Clean up and exit.
  ospRelease(lights[0]);
  ospRelease(renderer);
  ospRelease(frameBuffer);

  // If using the C++ API of OSPray, this will probably have to be wrapped in
  // a std::atexit in order for it to be called after the destructors of OSPRay
  // objects in this scope.
  ospShutdown();

  return EXIT_SUCCESS;
}

