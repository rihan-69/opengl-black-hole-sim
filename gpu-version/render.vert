#version 430 core

#define TRAIL_LENGTH 500

struct Photon {
    vec4 pos;
    vec4 vel;
    vec4 history[TRAIL_LENGTH];
    uint history_index;
    vec3 _padding;
};

layout(std430, binding = 0) buffer PhotonBuffer {
    Photon photons[];
};

uniform mat4 proj;
uniform mat4 view;

out float alpha_out;

void main() {
    uint photon_id = gl_VertexID / TRAIL_LENGTH;
    uint trail_id = gl_VertexID % TRAIL_LENGTH;

    uint history_idx = (photons[photon_id].history_index + trail_id) % TRAIL_LENGTH;
    
    vec4 pos = photons[photon_id].history[history_idx];
    
    gl_Position = proj * view * vec4(pos.xyz, 1.0);
    
    // Reverted to the previous, thinner point size
    gl_PointSize = 1.0 + (float(trail_id) / TRAIL_LENGTH) * 2.0;
    
    alpha_out = float(trail_id) / TRAIL_LENGTH;
}
