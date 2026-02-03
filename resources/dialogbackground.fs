#version 330

out vec4 finalColor;

uniform float time;
uniform vec2 resolution;
uniform vec4 color1;
uniform vec4 color2;

void main() {
    vec2 uv = gl_FragCoord.xy / resolution.x;
    uv += time / 20;
    
    float scale = 48.0;
    uv *= scale;
    vec4 grid = int(floor(uv.x) + floor(uv.y)) % 2 == 0 ? color1 : color2;

    finalColor = vec4(vec3(grid), 1.0);
}