struct CubeFragUniforms {
    int frame_index;
    float opacity;
    int frame_count;
    float u_time;
};

float3 cube_rotY(float3 p, float a) {
    float c = cos(a), s = sin(a);
    return float3(p.x * c + p.z * s, p.y, -p.x * s + p.z * c);
}

float3 cube_rotX(float3 p, float a) {
    float c = cos(a), s = sin(a);
    return float3(p.x, p.y * c - p.z * s, p.y * s + p.z * c);
}

float3 cube_rotZ(float3 p, float a) {
    float c = cos(a), s = sin(a);
    return float3(p.x * c - p.y * s, p.x * s + p.y * c, p.z);
}

float2 cube_boxIntersect(float3 ro, float3 rd, float3 h) {
    float3 inv = 1.0 / rd;
    float3 t1 = (-h - ro) * inv;
    float3 t2 = ( h - ro) * inv;
    float3 tmin = min(t1, t2);
    float3 tmax = max(t1, t2);
    float near_t = max(max(tmin.x, tmin.y), tmin.z);
    float far_t  = min(min(tmax.x, tmax.y), tmax.z);
    return float2(near_t, far_t);
}

float3 cube_boxNormal(float3 p, float3 h) {
    if (abs(abs(p.x) - h.x) < 0.001) return float3(sign(p.x), 0, 0);
    if (abs(abs(p.y) - h.y) < 0.001) return float3(0, sign(p.y), 0);
    return float3(0, 0, sign(p.z));
}

float2 cube_mapUV(float3 p, float3 n, float3 h) {
    float2 uv;
    if (abs(n.x) > 0.5) {
        uv = float2(p.z * -n.x, p.y) / h.xy;
    } else if (abs(n.y) > 0.5) {
        uv = float2(p.x, p.z * -n.y) / h.xy;
    } else {
        uv = float2(p.x * n.z, p.y) / h.xy;
    }
    return uv * 0.5 + 0.5;
}

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              texture2d<float> tex [[texture(0)]],
                              sampler samp [[sampler(0)]],
                              constant CubeFragUniforms& u [[buffer(0)]]) {
    float2 screen = in.texcoord * 2.0 - 1.0;
    screen.x *= 1.333;

    float3 ro = float3(0.0, 0.0, 2.8);
    float3 rd = normalize(float3(screen, -1.5));

    float ay = u.u_time * 0.7;
    float ax = u.u_time * 0.5;
    float az = u.u_time * 0.3;

    float3 rro = cube_rotZ(cube_rotX(cube_rotY(ro, -ay), -ax), -az);
    float3 rrd = cube_rotZ(cube_rotX(cube_rotY(rd, -ay), -ax), -az);

    float3 half_size = float3(0.7, 0.7, 0.7);
    float2 t = cube_boxIntersect(rro, rrd, half_size);

    if (t.x > t.y || t.y < 0.0)
        discard_fragment();

    float hit_t = t.x > 0.0 ? t.x : t.y;
    float3 hit = rro + rrd * hit_t;
    float3 n = cube_boxNormal(hit, half_size);
    float2 uv = cube_mapUV(hit, n, half_size);

    // World-space normal for lighting
    float3 world_n = cube_rotY(cube_rotX(cube_rotZ(n, az), ax), ay);

    // Phong shading
    float3 light_dir = normalize(float3(0.5, 1.0, 0.8));
    float3 view_dir = normalize(-rd);
    float3 reflect_dir = reflect(-light_dir, world_n);

    float ambient = 0.25;
    float diffuse = max(dot(world_n, light_dir), 0.0);
    float specular = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0);

    float lighting = ambient + 0.55 * diffuse + 0.4 * specular;

    // Force opaque background on cube faces
    float inv_count = 1.0 / float(u.frame_count);
    float2 atlas_uv = float2(uv.x, uv.y * inv_count + float(u.frame_index) * inv_count);
    float4 color = tex.sample(samp, atlas_uv);
    color.rgb = mix(float3(0.15, 0.15, 0.25), color.rgb, color.a);
    color.a = 1.0;

    color.rgb *= lighting;
    color.a *= u.opacity;

    return color;
}
