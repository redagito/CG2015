#version 330 core

uniform sampler2D scene_texture;
uniform float screen_width;
uniform float screen_height;

uniform float uDispersal = 0.25;
uniform int uGhosts = 8;
uniform vec4 uScale = vec4(1.0);
uniform vec4 uBias = vec4(0.0);

layout (location = 0) out vec4 color;

 void main() {

	// Calculate screen position of the fragment [0-1]
	vec2 screen_coords = vec2(gl_FragCoord.x / screen_width, gl_FragCoord.y / screen_height);

	color = max(vec4(0.0), texture(scene_texture, screen_coords) + uBias) * uScale;

   // ghost vector to image centre:
      vec2 ghostVec = (vec2(0.5) - screen_coords) * uDispersal;

	vec4 result = color;
      for (int i = 0; i < uGhosts; ++i) { 
         vec2 offset = fract(screen_coords + ghostVec * float(i));
  
         result += texture(scene_texture, offset);
      }

	color = result;
}