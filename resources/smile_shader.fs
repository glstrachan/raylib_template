#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 out_color;

uniform vec2 screenResolution;
uniform vec2 elementResolution;
uniform vec2 elementPosition;

void main() {
    vec2 uv = (gl_FragCoord.xy - elementPosition) / elementResolution;

    vec4 red = vec4(1, 0, 0, 1);
    vec4 green = vec4(0, 1, 0, 1);

    float t = 1.0 - uv.x;

    vec2 a = normalize(vec2(1 - t, t));
    out_color = a.x * green + a.y * red;
}