#ifndef GLFILTERS_HEADER
#define GLFILTERS_HEADER

#include <string>
#include <gst/gst.h>

void set_glgreyscale(GstElement* glbalance);

template<typename properties> void set_glundistort(GstElement* glundistort, const properties props) {
  const std::string shader = R"(#version 330 core
precision lowp float;

in vec2 v_texcoord;
out vec4 fragColor;

uniform sampler2D tex;

// Distortion parameters
uniform float k1;  // radial distortion coefficient
uniform float k2;  // optional higher-order term
uniform float scale; // zoom adjustment

void main()
{
    // Convert [0,1] → [-1,1]
    vec2 uv = v_texcoord * 2.0 - 1.0;

    // Apply scaling (useful to zoom in/out after correction)
    uv /= scale;

    // Inverse radial distortion (approximate)
    float r2 = dot(uv, uv);   // r^2
    float factor = 1.0 + k1 * r2 + k2 * r2 * r2;

    uv = uv * factor;

    // Convert back to [0,1]
    vec2 corrected_uv = uv * 0.5 + 0.5;

    fragColor = texture(tex, corrected_uv);
})";
  const std::string uniforms = "uniforms";
  GstStructure *str = gst_structure_new(
    uniforms.c_str(),
    "k1", G_TYPE_FLOAT, props->undistort_k1,
    "k2", G_TYPE_FLOAT, props->undistort_k2,
    "scale", G_TYPE_FLOAT, props->undistort_scale,
  NULL);
  g_object_set(glundistort,
    "fragment", shader.c_str(),
    "uniforms", str,
  NULL);
  gst_structure_free(str);
}

template<typename properties> void set_glcrop43(GstElement* glcrop, const properties props) { 
  g_object_set(glcrop,
    "scale-x", (double) props->width/(props->height/3*4),
  NULL);
};

#endif
