#version 300 es

precision mediump float;
in vec2 vTexCoord;

uniform sampler2D image;
uniform vec2 offset;

out vec4 fragColor;

void main() {
    //vec2 samplePoint = vTexCoord + offset;
    vec2 samplePoint = vec2(vTexCoord.x, 0.5);

    // Sample at the projected point
    // vec4 texCol = (texture(image, vTexCoord) + vec4(1.0)) * 0.5;
    vec4 col = texture(image, samplePoint);

    float brightness = sqrt(col.x * col.x + col.y * col.y + col.z * col.z) / 5.0;

    if (brightness > vTexCoord.y) {
        fragColor = vec4(1., 1., 1., 1.);
    }
    else {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        //fragColor = col;
    }

}
