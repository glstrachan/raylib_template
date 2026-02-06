#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 out_color;

uniform vec2 screenResolution;
uniform vec2 elementResolution;
uniform vec2 elementPosition;
uniform float completion;

void main() {
    vec2 fragCoord = (gl_FragCoord.xy);
    fragCoord.y = screenResolution.y - fragCoord.y;
    vec2 uv = (fragCoord - elementPosition) / elementResolution;
    uv = uv * 2.0 - 1.0;

    if (atan(uv.y, uv.x) + 3.1415 < (1.0 - completion) * 3.1415 * 2.0 && (uv.x*uv.x) + (uv.y*uv.y) < 0.9) {
        out_color = vec4(0, 100.0/255.0, 215.0/255.0, 1.0);
    } else {
        out_color = vec4(0);
    }
}