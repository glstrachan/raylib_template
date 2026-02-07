#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float iTime;
uniform sampler2D textureSampler;

out vec4 finalColor;

uniform vec2 screenResolution;
uniform vec2 elementResolution;
uniform vec2 elementPosition;

vec2 curve(vec2 uv)
{
	uv = (uv - 0.5) * 2.0;
	uv *= 1.1;	
	uv.x *= 1.0 + pow((abs(uv.y) / 5.0), 2.0);
	uv.y *= 1.0 + pow((abs(uv.x) / 4.0), 2.0);
	uv  = (uv / 2.0) + 0.5;
	uv =  uv *0.92 + 0.04;
	return uv;
}

void main1()
{
    //Curve
    vec2 uv = fragTexCoord.xy;
    // vec2 fragCoord = (gl_FragCoord.xy);
    // fragCoord.y = screenResolution.y - fragCoord.y;
    // vec2 uv = (fragCoord - elementPosition) / elementResolution;
    // vec2 uv = fragTexCoord;
	uv = curve( uv );
    
    vec3 col;

    // Chromatic
    col.r = texture(textureSampler,vec2(uv.x+0.003,uv.y)).x;
    col.g = texture(textureSampler,vec2(uv.x+0.000,uv.y)).y;
    col.b = texture(textureSampler,vec2(uv.x-0.003,uv.y)).z;

    col *= step(0.0, uv.x) * step(0.0, uv.y);
    col *= 1.0 - step(1.0, uv.x) * 1.0 - step(1.0, uv.y);

    col *= 0.5 + 0.5*16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y);
    col *= vec3(0.95,1.05,0.95);

    col *= 0.9+0.1*sin(10.0*iTime+uv.y*700.0);

    col *= 0.99+0.01*sin(110.0*iTime);

    // finalColor = vec4(uv, 0, 1);
    finalColor = vec4(col,1.0);
}

// void main() {
//     vec4 color = texture(textureSampler, fragTexCoord) * fragColor;
//     float gray = (color.r + color.g + color.b) / 3.0;
//     finalColor = color;
// }


void main()
{
    // vec2 uv = fragCoord / iResolution.xy;
    vec2 uv = fragTexCoord;
    
    // CRT barrel distortion parameters
    float barrelPower = 1.05;
    float chromaAmount = 0.006;
    
    // Apply barrel distortion inline
    vec2 centered = 2.0 * uv - 1.0;
    float theta = atan(centered.y, centered.x);
    float radius = length(centered);
    radius = pow(radius, barrelPower);
    centered.x = radius * cos(theta);
    centered.y = radius * sin(theta);
    vec2 distortedUV = 0.5 * (centered + 1.0);
    
    // Chromatic aberration - sample RGB channels at different offsets
    vec2 redOffset = (distortedUV - 0.5) * (1.0 + chromaAmount) + 0.5;
    vec2 greenOffset = distortedUV;
    vec2 blueOffset = (distortedUV - 0.5) * (1.0 - chromaAmount) + 0.5;
    
    // Sample each channel
    float r = texture(textureSampler, redOffset).r;
    float g = texture(textureSampler, greenOffset).g;
    float b = texture(textureSampler, blueOffset).b;
    
    vec3 color = vec3(r, g, b);
    
    // Add blur effect using low-quality blur from the shaders
    vec2 pixelSize = 1.0 / screenResolution;
    vec2 offset = vec2(1.333) * pixelSize * 1.5;
    
    vec3 blurColor = vec3(0.0);
    blurColor += color * 0.29411764705882354;
    blurColor += texture(textureSampler, distortedUV + offset).rgb * 0.35294117647058826;
    blurColor += texture(textureSampler, distortedUV - offset).rgb * 0.35294117647058826;
    
    // Mix original with blur
    color = mix(color, blurColor, 0.3);
    
    // Vignette effect
    vec2 vignetteUV = uv - 0.5;
    float vignette = 1.0 - length(vignetteUV) * 0.8;
    vignette = smoothstep(0.3, 0.9, vignette);
    color *= vignette;
    
    // Scanlines
    float scanline = sin(fragTexCoord.y * elementResolution.y * 2.0) * 0.04;
    color -= scanline;
    
    // CRT glow/bloom effect
    float brightness = (color.r + color.g + color.b) / 3.0;
    color += pow(brightness, 3.0) * 0.2;
    
    // Screen edge darkening
    float edge = 1.0;
    if (distortedUV.x < 0.0 || distortedUV.x > 1.0 || distortedUV.y < 0.0 || distortedUV.y > 1.0)
    {
        edge = 0.0;
    }
    
    // Smooth edges
    edge *= smoothstep(0.0, 0.02, distortedUV.x);
    edge *= smoothstep(1.0, 0.98, distortedUV.x);
    edge *= smoothstep(0.0, 0.02, distortedUV.y);
    edge *= smoothstep(1.0, 0.98, distortedUV.y);
    
    color *= edge;
    
    // Slight color grading for CRT look
    color = pow(color, vec3(1.0 / 2.2)); // Gamma correction
    color *= vec3(1.0, 0.98, 0.95); // Slight warm tint
    color = pow(color, vec3(2.2)); // Back to linear
    
    finalColor = vec4(color, 1.0);
}
