// Linear-light, energy-bounded Blinn-Phong response shared by opaque and blend paths.
vec3 safeDirection(vec3 v) { return v * inversesqrt(max(dot(v,v), 1e-12)); }
void surfaceLight(vec3 N, vec3 V, vec3 L, vec3 radiance, float strength, out vec3 diffuse, out vec3 specular)
{
    N = safeDirection(N); V = safeDirection(V); L = safeDirection(L);
    float cosine = max(dot(N,L),0.0);
    float f0 = 0.04 * clamp(strength,0.0,1.0);
    vec3 H = safeDirection(V+L);
    float highlight = pow(max(dot(N,H),0.0),32.0) * (40.0 / 25.13274123);
    diffuse = radiance * cosine * (1.0-f0);
    specular = radiance * cosine * f0 * highlight;
}
