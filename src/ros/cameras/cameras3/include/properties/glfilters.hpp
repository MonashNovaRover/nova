#ifndef GLFILTERS_HEADER
#define GLFILTERS_HEADER

#include <string>
#include <gst/gst.h>

void set_glgreyscale(GstElement* glgreyscale);

template<typename properties> void set_glantialias(GstElement* glantialias, const properties props) {
  // fxaa antiliasing
  const std::string shader = R"(#version 100
precision lowp float;

uniform float factor;
uniform float width;
uniform float height;
uniform sampler2D tex;
varying vec2 v_texcoord;

void main() {
    vec2 texel = factor / vec2(width, height); // 1 pixel in UV

    // Sample neighboring pixels (only 4 corners)
    vec3 c0 = texture2D(tex, v_texcoord).rgb;
    vec3 c1 = texture2D(tex, v_texcoord + vec2(-texel.x, 0)).rgb;
    vec3 c2 = texture2D(tex, v_texcoord + vec2(texel.x, 0)).rgb;
    vec3 c3 = texture2D(tex, v_texcoord + vec2(0, -texel.y)).rgb;
    vec3 c4 = texture2D(tex, v_texcoord + vec2(0, texel.y)).rgb;

    // Compute simple luminance differences
    vec3 horiz = (c1 + c2) * 0.5;
    vec3 vert  = (c3 + c4) * 0.5;

    // Blend based on which direction has more contrast
    float lumH = abs(dot(horiz - c0, vec3(0.299, 0.587, 0.114)));
    float lumV = abs(dot(vert  - c0, vec3(0.299, 0.587, 0.114)));

    gl_FragColor = vec4(mix(c0, (lumH > lumV) ? horiz : vert, 0.5), 1.0);
})";
  const std::string uniforms = "uniforms";
  GstStructure *str = gst_structure_new(
    uniforms.c_str(),
    "factor", G_TYPE_FLOAT, props->antialias_factor,
    "width", G_TYPE_FLOAT, ((float) props->width/ (float) props->downscale),
    "height", G_TYPE_FLOAT, ((float) props->height/ (float) props->downscale),
  NULL);
  g_object_set(glantialias,
    "fragment", shader.c_str(),
    "uniforms", str,
  NULL);
  gst_structure_free(str);
}


template<typename properties> void set_gledgedetect(GstElement* gledgedetect, const properties props) {
  // sobel edge detection
  const std::string shader = R"(#version 100
precision lowp float;

uniform float factor;
uniform float width;
uniform float height;
uniform sampler2D tex;
varying vec2 v_texcoord;

void main() {
    vec2 texOffset = factor / vec2(width, height);

    // Sample the 3x3 neighborhood
    vec3 c00 = texture2D(tex, v_texcoord + texOffset * vec2(-1.0, -1.0)).rgb;
    vec3 c10 = texture2D(tex, v_texcoord + texOffset * vec2( 0.0, -1.0)).rgb;
    vec3 c20 = texture2D(tex, v_texcoord + texOffset * vec2( 1.0, -1.0)).rgb;

    vec3 c01 = texture2D(tex, v_texcoord + texOffset * vec2(-1.0,  0.0)).rgb;
    vec3 c11 = texture2D(tex, v_texcoord + texOffset * vec2( 0.0,  0.0)).rgb;
    vec3 c21 = texture2D(tex, v_texcoord + texOffset * vec2( 1.0,  0.0)).rgb;

    vec3 c02 = texture2D(tex, v_texcoord + texOffset * vec2(-1.0,  1.0)).rgb;
    vec3 c12 = texture2D(tex, v_texcoord + texOffset * vec2( 0.0,  1.0)).rgb;
    vec3 c22 = texture2D(tex, v_texcoord + texOffset * vec2( 1.0,  1.0)).rgb;

    // Sobel kernels
    vec3 gx = -c00 - 2.0*c01 - c02 + c20 + 2.0*c21 + c22;
    vec3 gy = -c00 - 2.0*c10 - c20 + c02 + 2.0*c12 + c22;

    vec3 edge = sqrt(gx*gx + gy*gy);

    gl_FragColor = vec4(edge, 1.0);
})";
  const std::string uniforms = "uniforms";
  GstStructure *str = gst_structure_new(
    uniforms.c_str(),
    "factor", G_TYPE_FLOAT, props->edgedetect_factor,
    "width", G_TYPE_FLOAT, ((float) props->width/ (float) props->downscale),
    "height", G_TYPE_FLOAT, ((float) props->height/ (float) props->downscale),
  NULL);
  g_object_set(gledgedetect,
    "fragment", shader.c_str(),
    "uniforms", str,
  NULL);
  gst_structure_free(str);
}

template<typename properties> void set_glundistort(GstElement* glundistort, const properties props) {
  const std::string shader = R"(#version 100
precision lowp float;

uniform sampler2D tex;
varying vec2 v_texcoord;

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

    gl_FragColor = texture2D(tex, corrected_uv);
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
