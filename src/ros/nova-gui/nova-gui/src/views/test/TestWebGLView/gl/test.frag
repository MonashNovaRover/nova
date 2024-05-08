#version 300 es

precision mediump float;
in vec2 vTexCoord;

uniform sampler2D spec;
//uniform vec2 offset;

uniform vec2 resolution;

out vec4 fragColor;

void main() {
    //vec2 samplePoint = vTexCoord + offset;
    vec2 samplePoint = vec2(vTexCoord.x, 0.5);

    // Sample at the projected point
    // vec4 texCol = (texture(image, vTexCoord) + vec4(1.0)) * 0.5;
    vec4 col = texture(spec, samplePoint);

    float brightness = sqrt(col.x * col.x + col.y * col.y + col.z * col.z) / 1.73;

    if (brightness > vTexCoord.y) {
        if (mod(floor(vTexCoord.x * resolution.x) + floor(vTexCoord.y * resolution.y), 2.0) > 0.0) {
            fragColor = vec4(1.);
        }
        else {
            fragColor = texture(spec, vTexCoord * vec2(1., 1.) + vec2(0., 0.5));
        }
    }
    else {
        //fragColor = col;

        if (mod(floor(vTexCoord.x * resolution.x) + floor(vTexCoord.y * resolution.y), 2.0) > 0.0) {
            fragColor = vec4(0.8, 0.5, 0.55, 1.0);
        }
        else {
            fragColor = vec4(0.2, 0.0, 0.8, 1.0);
        }
    }

}
