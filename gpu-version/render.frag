#version 430 core

in float alpha_out;
out vec4 FragColor;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    float intensity = pow(max(0.0, 1.0 - dist / 0.5), 2.0);

    // Reverted to the Interstellar Color Palette
    vec3 core_color = vec3(1.0, 1.0, 0.8); 
    vec3 glow_color = vec3(1.0, 0.7, 0.2); 

    vec3 final_color = mix(glow_color, core_color, pow(intensity, 8.0));
    
    float final_alpha = intensity * alpha_out;

    FragColor = vec4(final_color, final_alpha);
}
