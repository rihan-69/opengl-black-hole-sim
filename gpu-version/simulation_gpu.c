#include <GL/glew.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define NUM_PHOTONS 2000
#define TRAIL_LENGTH 500

typedef struct { float x, y, z, w; } Vector4D;
typedef struct {
    Vector4D pos;
    Vector4D vel;
    Vector4D history[TRAIL_LENGTH];
    int history_index;
    float _padding[3];
} Photon;

GLuint compute_program, render_program, ssbo, vao, proj_loc, view_loc;

// Camera variables for 3D rotation and zoom
float cam_angle_x = 35.0f;
float cam_angle_z = -45.0f;
float cam_dist = 100.0f;

// Mouse state variables
int mouse_x_prev = 0;
int mouse_y_prev = 0;
int is_dragging = 0;

// --- FULL HELPER FUNCTIONS (NOT CONDENSED) ---
char* readFile(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Fatal Error: Unable to open file '%s'\n", filename);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = (char*)malloc(size + 1);
    fread(buffer, 1, size, f);
    fclose(f);
    buffer[size] = 0;
    return buffer;
}

void checkShaderError(GLuint shader, const char* name) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        fprintf(stderr, "Error compiling shader: %s\n", name);
        GLint logSize = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
        char* infoLog = (char*)malloc(logSize);
        glGetShaderInfoLog(shader, logSize, NULL, infoLog);
        fprintf(stderr, "%s\n", infoLog);
        free(infoLog);
        exit(1);
    }
}

void checkProgramError(GLuint program, const char* name) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        fprintf(stderr, "Error linking program: %s\n", name);
        GLint logSize = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
        char* infoLog = (char*)malloc(logSize);
        glGetProgramInfoLog(program, logSize, NULL, infoLog);
        fprintf(stderr, "%s\n", infoLog);
        free(infoLog);
        exit(1);
    }
}

void setupShaders() {
    char* compute_shader_source = readFile("compute.glsl");
    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs, 1, (const char**)&compute_shader_source, NULL);
    glCompileShader(cs);
    checkShaderError(cs, "compute.glsl");
    compute_program = glCreateProgram();
    glAttachShader(compute_program, cs);
    glLinkProgram(compute_program);
    checkProgramError(compute_program, "Compute Program");
    free(compute_shader_source);

    char* vertex_shader_source = readFile("render.vert");
    char* fragment_shader_source = readFile("render.frag");
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, (const char**)&vertex_shader_source, NULL);
    glCompileShader(vs);
    checkShaderError(vs, "render.vert");
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, (const char**)&fragment_shader_source, NULL);
    glCompileShader(fs);
    checkShaderError(fs, "render.frag");
    render_program = glCreateProgram();
    glAttachShader(render_program, vs);
    glAttachShader(render_program, fs);
    glLinkProgram(render_program);
    checkProgramError(render_program, "Render Program");
    free(vertex_shader_source);
    free(fragment_shader_source);

    proj_loc = glGetUniformLocation(render_program, "proj");
    view_loc = glGetUniformLocation(render_program, "view");
}

void update() {
    glUseProgram(compute_program);
    glDispatchCompute((NUM_PHOTONS / 256) + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glutPostRedisplay();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(render_program);

    float proj[16], view[16];
    float aspect = 1.0f, fov = 45.0f, near_plane = 1.0f, far_plane = 1000.0f;
    float f = 1.0f / tan(fov * M_PI / 360.0);
    memset(proj, 0, sizeof(proj));
    proj[0] = f / aspect;
    proj[5] = f;
    proj[10] = (far_plane + near_plane) / (near_plane - far_plane);
    proj[11] = -1.0f;
    proj[14] = (2.0f * far_plane * near_plane) / (near_plane - far_plane);

    // --- CORRECTED 3D "LOOK AT" CAMERA MATRIX ---
    // 1. Calculate camera position in a sphere around the origin
    float cam_x = cam_dist * sin(cam_angle_z * M_PI / 180.0) * cos(cam_angle_x * M_PI / 180.0);
    float cam_y = cam_dist * sin(cam_angle_x * M_PI / 180.0);
    float cam_z = cam_dist * cos(cam_angle_z * M_PI / 180.0) * cos(cam_angle_x * M_PI / 180.0);

    // 2. Build the matrix to "look at" the origin (0,0,0) from this position
    // This is the proper math for a look-at matrix.
    float F[3] = { -cam_x, -cam_y, -cam_z }; // Forward vector
    float F_len = sqrt(F[0]*F[0] + F[1]*F[1] + F[2]*F[2]);
    F[0]/=F_len; F[1]/=F_len; F[2]/=F_len;
    float UP_vec[3] = {0.0, 1.0, 0.0};
    float S[3] = { F[1]*UP_vec[2] - F[2]*UP_vec[1], F[2]*UP_vec[0] - F[0]*UP_vec[2], F[0]*UP_vec[1] - F[1]*UP_vec[0] }; // Side vector
    float S_len = sqrt(S[0]*S[0] + S[1]*S[1] + S[2]*S[2]);
    S[0]/=S_len; S[1]/=S_len; S[2]/=S_len;
    float U[3] = { S[1]*F[2] - S[2]*F[1], S[2]*F[0] - S[0]*F[2], S[0]*F[1] - S[1]*F[0] }; // New Up vector

    memset(view, 0, sizeof(view));
    view[0]=S[0]; view[4]=S[1]; view[8]=S[2];
    view[1]=U[0]; view[5]=U[1]; view[9]=U[2];
    view[2]=-F[0]; view[6]=-F[1]; view[10]=-F[2];
    view[12]=-(S[0]*cam_x + S[1]*cam_y + S[2]*cam_z);
    view[13]=-(U[0]*cam_x + U[1]*cam_y + U[2]*cam_z);
    view[14]=-(-F[0]*cam_x + -F[1]*cam_y + -F[2]*cam_z);
    view[15]=1.0;

    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj);
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, NUM_PHOTONS * TRAIL_LENGTH);

    glutSwapBuffers();
}

void mouseFunc(int button, int state, int x, int y) {
    if (button == 3) { cam_dist /= 1.05f; if (cam_dist < 10.0f) cam_dist = 10.0f; } 
    else if (button == 4) { cam_dist *= 1.05f; if (cam_dist > 400.0f) cam_dist = 400.0f; }

    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) { is_dragging = 1; mouse_x_prev = x; mouse_y_prev = y; } 
        else { is_dragging = 0; }
    }
}

void motionFunc(int x, int y) {
    if (is_dragging) {
        int delta_x = x - mouse_x_prev;
        int delta_y = y - mouse_y_prev;
        cam_angle_z += delta_x * 0.25f;
        cam_angle_x += delta_y * 0.25f;
        if (cam_angle_x < 5.0f) cam_angle_x = 5.0f;
        if (cam_angle_x > 175.0f) cam_angle_x = 175.0f;
        mouse_x_prev = x;
        mouse_y_prev = y;
    }
}

void init() {
    glewInit();
    srand(time(NULL));
    setupShaders();
    
    Photon* initial_photons = (Photon*)malloc(sizeof(Photon) * NUM_PHOTONS);
    for (int i=0; i < NUM_PHOTONS; i++) {
        initial_photons[i].pos.x = -60.0f;
        initial_photons[i].pos.y = 0.0f;
        initial_photons[i].pos.z = (float)(rand() % 120) - 60.0f;
        initial_photons[i].pos.w = 1.0f;
        initial_photons[i].vel.x = 0.8f;
        initial_photons[i].vel.y = 0.0f;
        initial_photons[i].vel.z = (float)(rand() % 100 - 50) / 400.0f;
        initial_photons[i].vel.w = 0.0f;
        initial_photons[i].history_index = 0;
        for (int j=0; j < TRAIL_LENGTH; j++) {
            initial_photons[i].history[j] = initial_photons[i].pos;
        }
    }
    
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Photon) * NUM_PHOTONS, initial_photons, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    free(initial_photons);

    glGenVertexArrays(1, &vao);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 800);
    glutCreateWindow("GPU Black Hole Simulation");

    init();

    glutDisplayFunc(display);
    glutIdleFunc(update);
    glutMouseFunc(mouseFunc);
    glutMotionFunc(motionFunc);
    
    glutMainLoop();
    return 0;
}
