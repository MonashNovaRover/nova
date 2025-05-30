#version 300 es

precision mediump float;
in vec4 aPosition;
//in float aLuminance;

uniform vec2 uWavelengthLimits;
uniform vec2 uRainbowConfig;

//out vec2 vTexCoord;
//out float vLuminance;
out vec3 vWavelengthColor;

/**
 * Taken from Earl F. Glynn's web page:
 * <a href="http://www.efg2.com/Lab/ScienceAndEngineering/Spectra.htm">Spectra Lab Report</a>
 */
vec3 waveLengthToRGB(float Wavelength) {
    float IntensityMax = uRainbowConfig[0];
    float Gamma = uRainbowConfig[1];
    float factor;
    float Red, Green, Blue;

    if((Wavelength >= 380.0) && (Wavelength < 440.0)) {
        Red = -(Wavelength - 440.0) / (440.0 - 380.0);
        Green = 0.0;
        Blue = 1.0;
    } else if((Wavelength >= 440.0) && (Wavelength < 490.0)) {
        Red = 0.0;
        Green = (Wavelength - 440.0) / (490.0 - 440.0);
        Blue = 1.0;
    } else if((Wavelength >= 490.0) && (Wavelength < 510.0)) {
        Red = 0.0;
        Green = 1.0;
        Blue = -(Wavelength - 510.0) / (510.0 - 490.0);
    } else if((Wavelength >= 510.0) && (Wavelength < 580.0)) {
        Red = (Wavelength - 510.0) / (580.0 - 510.0);
        Green = 1.0;
        Blue = 0.0;
    } else if((Wavelength >= 580.0) && (Wavelength < 645.0)) {
        Red = 1.0;
        Green = -(Wavelength - 645.0) / (645.0 - 580.0);
        Blue = 0.0;
    } else if((Wavelength >= 645.0) && (Wavelength < 781.0)) {
        Red = 1.0;
        Green = 0.0;
        Blue = 0.0;
    } else {
        Red = 0.0;
        Green = 0.0;
        Blue = 0.0;
    }

    // Let the intensity fall off near the vision limits
    if((Wavelength >= 380.0) && (Wavelength < 420.0)) {
        factor = 0.3 + 0.7 * (Wavelength - 380.0) / (420.0 - 380.0);
    } else if((Wavelength >= 420.0) && (Wavelength < 701.0)) {
        factor = 1.0;
    } else if((Wavelength >= 701.0) && (Wavelength < 781.0)) {
        factor = 0.3 + 0.7 * (780.0 - Wavelength) / (780.0 - 700.0);
    } else {
        factor = 0.0;
    }

    vec3 rgb;

    // Don't want 0^x = 1 for x <> 0
    rgb[0] = IntensityMax * pow(Red * factor, Gamma);
    rgb[1] = IntensityMax * pow(Green * factor, Gamma);
    rgb[2] = IntensityMax * pow(Blue * factor, Gamma);

    return rgb;
}

void main() {
    float t = 0.5*aPosition[0] + 0.5;
    float wavelength = (1.0 - t) * uWavelengthLimits[0] + t * uWavelengthLimits[1];

    gl_Position = aPosition;
    // vTexCoord = vec2(0.5) + 0.5*aPosition.xy;
    // vLuminance = aLuminance;

    vWavelengthColor = waveLengthToRGB(wavelength);
}