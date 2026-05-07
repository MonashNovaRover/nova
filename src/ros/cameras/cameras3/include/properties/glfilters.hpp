#ifndef GLFILTERS_HEADER
#define GLFILTERS_HEADER

#include <string>
#include <gst/gst.h>

void set_glgreyscale(GstElement* glgreyscale);

void set_no_glgreyscale(GstElement* glgreyscale);

void set_no_glshader(GstElement* glshader);

template<typename properties> void set_glshaders(GstElement* element, const properties& props) {
  const std::string shader = R"(#version 100
precision mediump float;

uniform sampler2D tex;
varying vec2 v_texcoord;

uniform float width;
uniform float height;

uniform float undistort_k1;  // radial distortion coefficient
uniform float undistort_k2;  // optional higher-order term
uniform float undistort_scale; // zoom adjustment

uniform float denoise_factor;     // default 0.5
uniform float denoise_sigma;      // spatial sigma (e.g. 2.0)
uniform float denoise_threshold;  // color similarity threshold (e.g. 0.1)
uniform int denoise_radius;       // Default 3

uniform float edgedetect_factor;  // Default 2.0

uniform int do_undistort;
uniform int do_greyscale;
uniform int do_denoise;
uniform int do_edgedetect;

// 1D Bilateral Function
vec3 computeBlur(vec3 centerColor, vec2 direction) {
  vec3 sum = vec3(0.0);
  float weightSum = 0.0;

  for (int i = -7; i <= 7; i++) {
    if (i < -denoise_radius) continue;
    if (i > denoise_radius) break;

    vec2 offset = direction * float(i);
    vec3 sampleColor = texture2D(tex, v_texcoord + offset).rgb;

    float spatialDist = float(abs(i));
    float colorDist = length(sampleColor - centerColor);

    // Simple Gaussian approximation
    float w = exp(-(spatialDist * spatialDist) / (2.0 * denoise_sigma * denoise_sigma)) * exp(-(colorDist * colorDist) / (2.0 * denoise_threshold * denoise_threshold));

    sum += sampleColor * w;
    weightSum += w;
  }
  return sum / weightSum;
}

void main() {

  vec2 uv = v_texcoord;

  if (do_undistort == 1) {
    uv = v_texcoord * 2.0 - 1.0;
    uv /= undistort_scale;
    float r2 = dot(uv, uv);   // r^2
    float undistort_factor = 1.0 + undistort_k1 * r2 + undistort_k2 * r2 * r2;

    uv *= undistort_factor;
    uv = uv * 0.5 + 0.5;
  }

  vec3 centerColor = texture2D(tex, uv).rgb;
  vec3 result = centerColor;

  if (do_denoise == 1) {
    
    // Direction vectors
    vec2 hDir = vec2(1.0 / width, 0.0);
    vec2 vDir = vec2(0.0, 1.0 / height);

    // Apply both
    vec3 hBlur = computeBlur(centerColor, hDir);
    vec3 vBlur = computeBlur(centerColor, vDir);

    // Average them or combine
    result = result * ((hBlur + vBlur) * 0.5) * denoise_factor + centerColor * (1.0 - denoise_factor);
  }

  if (do_edgedetect == 1) {
    vec2 texOffset = edgedetect_factor / vec2(width, height);

    // Sample the 3x3 neighborhood
    vec3 c00 = texture2D(tex, uv + texOffset * vec2(-1.0, -1.0)).rgb;
    vec3 c10 = texture2D(tex, uv + texOffset * vec2( 0.0, -1.0)).rgb;
    vec3 c20 = texture2D(tex, uv + texOffset * vec2( 1.0, -1.0)).rgb;

    vec3 c01 = texture2D(tex, uv + texOffset * vec2(-1.0,  0.0)).rgb;
    vec3 c11 = texture2D(tex, uv + texOffset * vec2( 0.0,  0.0)).rgb;
    vec3 c21 = texture2D(tex, uv + texOffset * vec2( 1.0,  0.0)).rgb;

    vec3 c02 = texture2D(tex, uv + texOffset * vec2(-1.0,  1.0)).rgb;
    vec3 c12 = texture2D(tex, uv + texOffset * vec2( 0.0,  1.0)).rgb;
    vec3 c22 = texture2D(tex, uv + texOffset * vec2( 1.0,  1.0)).rgb;

    // Sobel kernels
    vec3 gx = -c00 - 2.0*c01 - c02 + c20 + 2.0*c21 + c22;
    vec3 gy = -c00 - 2.0*c10 - c20 + c02 + 2.0*c12 + c22;

    result = result * sqrt(gx*gx + gy*gy);
  }

  if (do_greyscale == 1) {
    float grey = dot(result, vec3(0.299, 0.587, 0.114));
    result = vec3(grey);
  }

  gl_FragColor = vec4(result, 1.0);
})";
  const std::string uniforms = "uniforms";
  GstStructure *str = gst_structure_new(
    uniforms.c_str(),
    "width", G_TYPE_FLOAT, ((float) props->width/ (float) props->downscale),
    "height", G_TYPE_FLOAT, ((float) props->height/ (float) props->downscale),
    "undistort_k1", G_TYPE_FLOAT, props->undistort_k1,
    "undistort_k2", G_TYPE_FLOAT, props->undistort_k2,
    "undistort_scale", G_TYPE_FLOAT, props->undistort_scale,
    "denoise_factor", G_TYPE_FLOAT, props->denoise_factor,
    "denoise_sigma", G_TYPE_FLOAT, props->denoise_sigma,
    "denoise_threshold", G_TYPE_FLOAT, props->denoise_threshold,
    "denoise_radius", G_TYPE_INT, props->denoise_radius,
    "edgedetect_factor", G_TYPE_FLOAT, props->edgedetect_factor,
    "do_undistort", G_TYPE_INT, (int) props->undistort,
    "do_greyscale", G_TYPE_INT, (int) props->greyscale,
    "do_denoise", G_TYPE_INT, (int) props->denoise,
    "do_edgedetect", G_TYPE_INT, (int) props->edgedetect,
  NULL);
  g_object_set(element,
    "fragment", shader.c_str(),
    "uniforms", str,
  NULL);
  gst_structure_free(str);
}

template<typename properties> void set_glcrop43(GstElement* element, const properties& props) { 
  g_object_set(element,
    "scale-x", (double) props->width/(props->height/3*4),
  NULL);
};

void set_no_glcrop43(GstElement* element);

#endif
