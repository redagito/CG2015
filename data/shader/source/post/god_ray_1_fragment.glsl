#version 460 core
uniform sampler2D depth_texture;
uniform vec2 light_position_screen;
uniform bool sun_active=false;
uniform vec3 sun_color=vec3(1);
uniform float screen_width,screen_height;
layout(location=0) out vec3 color;
float source(vec2 coord) {
    if(any(lessThan(coord,vec2(0))) || any(greaterThan(coord,vec2(1)))) return 0.0;
    vec2 delta=coord-light_position_screen; delta.x*=screen_width/screen_height;
    float disc=1.0-smoothstep(0.008,0.02,length(delta));
    return texture(depth_texture,coord).r>=0.999999 ? disc : 0.0;
}
void main() {
    color=vec3(0); if(!sun_active) return;
    vec2 uv=gl_FragCoord.xy/vec2(screen_width,screen_height),step=(uv-light_position_screen)*0.96/100.0;
    float sum=0.0,decay=1.0;
    for(int i=0;i<100;++i) {uv-=step;sum+=source(uv)*decay*0.05;decay*=0.97;}
    color=sun_color*sum;
}
