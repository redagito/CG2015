#version 460 core
in vec2 uv;
in vec4 tint;
flat in int particle_kind;
layout(location=0) out vec4 color;
void main()
{
    float radius=length(uv),coverage=0;
    if(particle_kind==6)
        coverage=pow(max(0,1-abs(uv.y)),1.5)*(1-smoothstep(.97,1.0,abs(uv.x)));
    else if(particle_kind==1)
        coverage=(1-smoothstep(.1,1.0,abs(uv.y)))*(1-smoothstep(.3,1.0,abs(uv.x)));
    else if(particle_kind==3) {
        if(uv.y < -.8+abs(uv.x)*1.3)discard;
        coverage=1-smoothstep(.7,1.0,max(abs(uv.x),abs(uv.y)));
    } else {
        if(radius>=1)discard;
        if(particle_kind==2) {
            float rim=pow(1-sqrt(max(0,1-radius*radius)),2);
            float grid=.8+.2*cos(uv.x*28)*cos(uv.y*28);
            coverage=(.035+rim*grid)*(1-smoothstep(.94,1.0,radius));
        } else if(particle_kind==5) coverage=1-smoothstep(.025,.15,abs(radius-.8));
        else coverage=pow(max(0,1-radius*radius),particle_kind==4 ? 1.3 : 2.0);
    }
    if(coverage<.002)discard;
    color=vec4(tint.rgb,tint.a*coverage);
}
