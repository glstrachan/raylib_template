#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 out_color;

uniform float smile_0to1;
uniform vec2 resolution;

void main() {
    // out_color = vec4(1, 0, 0, 1);
    vec4 red = vec4(1, 0, 0, 1);
    vec4 green = vec4(0, 1, 0, 1);

    float i = abs(fragTexCoord.y - smile_0to1);
    // out_color = mix(green, red, i * 4.0);

    // float t = pow(i * 8.0, 0.3) * pow(i * 5, 0.5);
    float t = pow(i, 0.7)  * 4.0;
    // float t = i * 8.0;

    out_color = (1.5 - t) * green + t * red;
    // out_color = mix(green, red, );
    // out_color = mix(red, green, pow(0.005, i));

    // out_color.g += fragTexCoord.x * 0.1;

    // out_color = vec4(0.0, fragTexCoord.y, 0, 1);
    // out_color = vec4(gl_FragCoord.xy / resolution.x, 0, 1);
}