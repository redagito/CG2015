// Mesh-local masks stay attached to turning/instanced ships and ignore UV seams.
in vec3 accentPositionModelSpace, accentNormalModelSpace;
uniform vec4 accent_color_strength = vec4(0);
// offset from center, strip half width, longitudinal start/end
uniform vec4 accent_hull_strips;
// rearmost region's forward limit, half width, vertical center/half height
uniform vec4 accent_engines;
float accentBand(float coordinate, float halfWidth)
{
    float edge = max(fwidth(coordinate), 0.0001);
    return 1.0 - smoothstep(halfWidth - edge, halfWidth + edge, abs(coordinate));
}
void applyEmissiveAccents(inout vec3 base, inout float glow)
{
    if (accent_color_strength.a <= 0.0) return;
    vec3 p = accentPositionModelSpace;
    vec3 n = normalize(accentNormalModelSpace);
    // Two longitudinal stripes on the upper and lower hull, clear of the nose.
    float center = (accent_hull_strips.z + accent_hull_strips.w) * 0.5;
    float length = (accent_hull_strips.w - accent_hull_strips.z) * 0.5;
    float hull = accentBand(abs(p.x) - accent_hull_strips.x, accent_hull_strips.y) *
                 accentBand(p.z - center, length) * smoothstep(0.2, 0.5, abs(n.y));
    // Only rear-facing panels emit engine light; the rest of the stern stays dark.
    float edge = max(fwidth(p.z), 0.0001);
    float engine = (1.0 - smoothstep(accent_engines.x - edge, accent_engines.x + edge, p.z)) *
                   accentBand(p.x, accent_engines.y) *
                   accentBand(p.y - accent_engines.z, accent_engines.w) *
                   smoothstep(0.35, 0.75, -n.z);
    float mask = clamp(max(hull, engine), 0.0, 1.0);
    // Tint the filtered edge before it blooms, keeping gray hull texels out of the red halo.
    float paint = smoothstep(0.0, 0.125, mask);
    base = mix(base, accent_color_strength.rgb, paint);
    glow = max(glow, mask * accent_color_strength.a);
}
