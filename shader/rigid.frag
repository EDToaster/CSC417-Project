#version 450

layout(location = 0) out vec4 color;

uniform vec2 viewportSize;

float PHI = 1.61803398874989484820459;  // phi = Golden Ratio   
float PHIM1 = 0.61803398874989484820459;  // phi = Golden Ratio   

float gold_noise(in vec2 xy, in float seed){
	return fract(tan(distance(xy*PHI, xy)*seed)*xy.x);
}

float gold_noise(in float x, in float seed) {
	return fract(tan(x * PHIM1 *seed) * x);
}

vec4 render_noise(in uint ind, in vec4 col) {
	return clamp(gold_noise(ind, 1), 0, 1) * col;
}

void main() {
	// Calculate texture coordinates from gl_FragCoord
	vec2 texCoord = gl_FragCoord.xy / viewportSize;
	color = vec4(0.7, 0.5, 0.4, 1) + render_noise(uint(10 * texCoord.x) + 10 * uint(10 * texCoord.y), vec4(0.2, 0.2, 0.2, 0.0));
}