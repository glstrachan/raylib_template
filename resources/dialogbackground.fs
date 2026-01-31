#version 330

out vec4 finalColor;

uniform float time;
uniform vec2 resolution;

void main() {
    vec2 uv = gl_FragCoord.xy / resolution.x;
    uv += time / 20;
    
    float scale = 16.0;
    uv *= scale;
    float grid = int(floor(uv.x) + floor(uv.y)) % 2 == 0 ? 0.196 : 0.184;

    finalColor = vec4(vec3(grid), 1.0);
}