#version 450

out vec3 color;
uniform sampler2D renderedTexture;
uniform vec2 viewportSize;

void main() {
    // Calculate texture coordinates from gl_FragCoord for full-screen quad
    vec2 texCoord = gl_FragCoord.xy / viewportSize;
    color = texture(renderedTexture, texCoord).xyz;
}