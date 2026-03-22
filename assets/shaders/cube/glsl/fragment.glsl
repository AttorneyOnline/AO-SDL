#version 330
layout (location = 0) out vec4 frag_color;

in vec2 vert_texcoord;

uniform sampler2DArray texture_sample;
uniform int frame_index;
uniform float opacity;
uniform float u_time;

vec3 rotY(vec3 p, float a) {
    float c = cos(a), s = sin(a);
    return vec3(p.x * c + p.z * s, p.y, -p.x * s + p.z * c);
}

vec3 rotX(vec3 p, float a) {
    float c = cos(a), s = sin(a);
    return vec3(p.x, p.y * c - p.z * s, p.y * s + p.z * c);
}

vec3 rotZ(vec3 p, float a) {
    float c = cos(a), s = sin(a);
    return vec3(p.x * c - p.y * s, p.x * s + p.y * c, p.z);
}

vec2 boxIntersect(vec3 ro, vec3 rd, vec3 h) {
    vec3 inv = 1.0 / rd;
    vec3 t1 = (-h - ro) * inv;
    vec3 t2 = ( h - ro) * inv;
    vec3 tmin = min(t1, t2);
    vec3 tmax = max(t1, t2);
    float near = max(max(tmin.x, tmin.y), tmin.z);
    float far  = min(min(tmax.x, tmax.y), tmax.z);
    return vec2(near, far);
}

vec3 boxNormal(vec3 p, vec3 h) {
    if (abs(abs(p.x) - h.x) < 0.001) return vec3(sign(p.x), 0, 0);
    if (abs(abs(p.y) - h.y) < 0.001) return vec3(0, sign(p.y), 0);
    return vec3(0, 0, sign(p.z));
}

vec2 cubeUV(vec3 p, vec3 n, vec3 h) {
    vec2 uv;
    if (abs(n.x) > 0.5) {
        uv = vec2(p.z * -n.x, p.y) / h.xy;
    } else if (abs(n.y) > 0.5) {
        uv = vec2(p.x, p.z * -n.y) / h.xy;
    } else {
        uv = vec2(p.x * n.z, p.y) / h.xy;
    }
    return uv * 0.5 + 0.5;
}

void main() {
    vec2 screen = vert_texcoord * 2.0 - 1.0;
    screen.x *= 1.333;

    vec3 ro = vec3(0.0, 0.0, 2.8);
    vec3 rd = normalize(vec3(screen, -1.5));

    // Tumble: rotate around all three axes at different speeds
    // so every face is visible over time
    float ay = u_time * 0.7;
    float ax = u_time * 0.5;
    float az = u_time * 0.3;

    // Rotate ray into cube's local space
    vec3 rro = rotZ(rotX(rotY(ro, -ay), -ax), -az);
    vec3 rrd = rotZ(rotX(rotY(rd, -ay), -ax), -az);

    vec3 half_size = vec3(0.7, 0.7, 0.7);
    vec2 t = boxIntersect(rro, rrd, half_size);

    if (t.x > t.y || t.y < 0.0)
        discard;

    float hit_t = t.x > 0.0 ? t.x : t.y;
    vec3 hit = rro + rrd * hit_t;
    vec3 n = boxNormal(hit, half_size);
    vec2 uv = cubeUV(hit, n, half_size);

    // Transform normal back to world space for lighting
    vec3 world_n = rotY(rotX(rotZ(n, az), ax), ay);

    // Phong shading
    vec3 light_dir = normalize(vec3(0.5, 1.0, 0.8));
    vec3 view_dir = normalize(-rd);
    vec3 reflect_dir = reflect(-light_dir, world_n);

    float ambient = 0.25;
    float diffuse = max(dot(world_n, light_dir), 0.0);
    float specular = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0);

    float lighting = ambient + 0.55 * diffuse + 0.4 * specular;

    // Sample texture — force opaque background on the cube faces
    vec4 tex_color = texture(texture_sample, vec3(uv, float(frame_index)));
    tex_color.rgb = mix(vec3(0.15, 0.15, 0.25), tex_color.rgb, tex_color.a);
    tex_color.a = 1.0;

    tex_color.rgb *= lighting;
    tex_color.a *= opacity;

    frag_color = tex_color;
}
