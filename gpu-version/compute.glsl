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

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

// Constants
const float GRID_SIZE = 40.0;
const float CAPTURE_RADIUS = 1.5;
const float PHOTON_SPHERE_RADIUS = 2.5;
const float TRAPPING_ZONE_WIDTH = 0.2;
const float BLACK_HOLE_MASS = 25.0;
const float PHOTON_SPEED = 0.8;
const float DT = 0.016; // Fixed timestep on GPU

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= photons.length()) return;

    vec2 current_pos = photons[id].pos.xz;
    float r_sq = dot(current_pos, current_pos);

    // Reset condition
    if (r_sq < CAPTURE_RADIUS * CAPTURE_RADIUS || r_sq > (GRID_SIZE * 1.5) * (GRID_SIZE * 1.5)) {
        photons[id].pos.x = -GRID_SIZE;
        photons[id].pos.z = -GRID_SIZE + (id / float(photons.length())) * (GRID_SIZE * 2.0);
        photons[id].vel.x = PHOTON_SPEED;
        photons[id].vel.z = 0.0;
    }

    float r = sqrt(r_sq);

    // Trapping logic
    if (r > PHOTON_SPHERE_RADIUS - TRAPPING_ZONE_WIDTH && r < PHOTON_SPHERE_RADIUS + TRAPPING_ZONE_WIDTH) {
        vec2 pos_vec = photons[id].pos.xz;
        vec2 vel_vec = photons[id].vel.xz;
        float radial_speed = dot(vel_vec, normalize(pos_vec));
        vec2 radial_vel = radial_speed * normalize(pos_vec);
        vec2 tangential_vel = vel_vec - radial_vel;
        photons[id].vel.xz = tangential_vel + radial_vel * 0.1;
    }

    // Gravity calculation
    if (r > 0.1) {
        vec2 accel = -BLACK_HOLE_MASS * normalize(photons[id].pos.xz) / r_sq;
        photons[id].vel.xz += accel * DT * 10.0;
    }

    photons[id].pos.xz += photons[id].vel.xz * DT * 20.0;
    
    // Update history
    photons[id].history[photons[id].history_index] = photons[id].pos;
    photons[id].history_index = (photons[id].history_index + 1) % TRAIL_LENGTH;
}
