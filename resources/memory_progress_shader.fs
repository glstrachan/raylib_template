#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform float progress;

void main() {
    float x = fragTexCoord.x;

    vec3 red = vec3(1.0, 0.0, 0.0);
    vec3 green = vec3(0.0, 1.0, 0.0);
    vec3 color = mix(red, green, x);

    vec2 a = normalize(vec2(1 - x, x));
    vec3 b = a.x * red + a.y * green;

    float edgeWidth = 0.005;
    float t = smoothstep(progress - edgeWidth, progress + edgeWidth, x);
    vec3 c = mix(b, vec3(0.5), t);

    finalColor = vec4(c, 1.0);
}