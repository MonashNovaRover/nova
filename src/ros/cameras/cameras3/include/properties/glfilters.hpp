#ifndef GLFILTERS_HEADER
#define GLFILTERS_HEADER

#include <string>
#include <gst/gst.h>

void set_glgreyscale(GstElement* glgreyscale);

template<typename properties> void set_gldenoise(GstElement* gldenoise, const properties& props) {
  // fxaa antiliasing
  const std::string shader = R"(#version 100
precision lowp float;

uniform sampler2D tex;
uniform float width;
uniform float height;
uniform float factor;     // default 1
uniform float sigma;      // spatial sigma (e.g. 2.0)
uniform float threshold;  // color similarity threshold (e.g. 0.1)
uniform int radius;     // Default 3

varying vec2 v_texcoord;

float gaussian(float x, float s) {
    return exp(-(x * x) / (2.0 * s * s));
}

void main() {
    vec2 texel = factor / vec2(width, height);

    vec3 centerColor = texture2D(tex, v_texcoord).rgb;

    vec3 sum = vec3(0.0);
    float weightSum = 0.0;

    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {

            vec2 offset = vec2(float(x), float(y)) * texel;
            vec3 sampleColor = texture2D(tex, v_texcoord + offset).rgb;

            float spatialDist = length(vec2(float(x), float(y)));
            float colorDist = length(sampleColor - centerColor);

            float wSpatial = gaussian(spatialDist, sigma);
            float wColor   = gaussian(colorDist, threshold);

            float weight = wSpatial * wColor;

            sum += sampleColor * weight;
            weightSum += weight;
        }
    }

    vec3 result = sum / weightSum;

    gl_FragColor = vec4(result, 1.0);
})";
  const std::string uniforms = "uniforms";
  GstStructure *str = gst_structure_new(
    uniforms.c_str(),
    "factor", G_TYPE_FLOAT, props->denoise_factor,
    "width", G_TYPE_FLOAT, ((float) props->width/ (float) props->downscale),
    "height", G_TYPE_FLOAT, ((float) props->height/ (float) props->downscale),
    "factor", G_TYPE_FLOAT, props->denoise_factor,
    "sigma", G_TYPE_FLOAT, props->denoise_factor,
    "threshold", G_TYPE_FLOAT, props->denoise_factor,
    "radius", G_TYPE_INT, props->denoise_radius,
  NULL);
  g_object_set(gldenoise,
    "fragment", shader.c_str(),
    "uniforms", str,
  NULL);
  gst_structure_free(str);
}


template<typename properties> void set_gledgedetect(GstElement* gledgedetect, const properties& props) {
  // sobel edge detection
  const std::string shader = R"(#version 100
precision lowp float;

uniform sampler2D tex;
uniform float factor;
uniform float width;
uniform float height;
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

template<typename properties> void set_glundistort(GstElement* glundistort, const properties& props) {
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

template<typename properties> void set_glcrop43(GstElement* glcrop, const properties& props) { 
  g_object_set(glcrop,
    "scale-x", (double) props->width/(props->height/3*4),
  NULL);
};

#endif
