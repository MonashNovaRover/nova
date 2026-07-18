#version 300 es

precision mediump float;
in vec4 aPosition;
in vec4 aLinePosition;

uniform vec2 resolution;
uniform vec2 imageResolution;
uniform float time;

out vec2 vTexCoord;


vec2 to_aspect(vec2 res) {
    return vec2(res.x/res.y, 1.0);
}

void main() {

    vec2 aspect = to_aspect(resolution);
    vec2 imageAspect = to_aspect(imageResolution);

    vec2 scale = aspect / imageAspect;
    if (scale.x > 1.0)
            scale = scale / vec2(scale.x);

    gl_Position = vec4(aPosition.xy , aPosition.zw);
    vTexCoord = (0.5 + 0.5*(aPosition.xy) /scale.yx + vec2(time, 0.));
}
