// cpu-version/simulation_cpu.c
#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define NUM_STARS 500
#define GRID_SIZE 40.0f
#define GRID_SLICES 50
#define NUM_PHOTONS 150
#define TRAIL_LENGTH 300
#define NUM_ORBITING_PHOTONS 40

typedef struct { float x, y, z; } Vector3D;
typedef struct { float x, z; } Vector2D;

Vector3D stars[NUM_STARS];

typedef struct {
    Vector3D pos;
    Vector2D vel;
    int active;
    Vector3D history[TRAIL_LENGTH];
    int history_index;
    float respawn_timer;
} Photon;

Photon photons[NUM_PHOTONS];

float get_y_displacement(float x, float z);
void reset_photon(int i, int is_orbiting);

float cam_angle_x = 35.0f;
float cam_angle_z = -45.0f;
float cam_dist = 100.0f;
const float BLACK_HOLE_MASS = 25.0f;

int mouse_x_prev = 0;
int mouse_y_prev = 0;
int is_dragging = 0;

void reset_photon(int i, int is_orbiting) {
    if (is_orbiting) {
        float angle = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
        float radius = 2.5f;
        photons[i].pos.x = radius * cos(angle);
        photons[i].pos.z = radius * sin(angle);
        photons[i].pos.y = get_y_displacement(photons[i].pos.x, photons[i].pos.z);
        float orbital_speed = sqrt(BLACK_HOLE_MASS / radius);
        photons[i].vel.x = -sin(angle) * orbital_speed;
        photons[i].vel.z = cos(angle) * orbital_speed;
    } else {
        photons[i].pos.x = -GRID_SIZE;
        photons[i].pos.z = (float)(rand() % (int)(GRID_SIZE * 2)) - GRID_SIZE;
        photons[i].pos.y = get_y_displacement(photons[i].pos.x, photons[i].pos.z);
        photons[i].vel.x = 0.8f;
        photons[i].vel.z = (float)(rand() % 100 - 50) / 400.0f;
    }
    photons[i].active = 1;
    photons[i].respawn_timer = 0.0f;
    for(int j = 0; j < TRAIL_LENGTH; j++) photons[i].history[j] = photons[i].pos;
    photons[i].history_index = 0;
}

float get_y_displacement(float x, float z) {
    float r = sqrt(x * x + z * z);
    if (r < 1.5f) r = 1.5f;
    return -BLACK_HOLE_MASS / r;
}

void update(int value) {
    float dt = 0.06f;
    const float max_force = 5.0f;
    for (int i = 0; i < NUM_PHOTONS; i++) {
        if (!photons[i].active) {
            photons[i].respawn_timer -= dt;
            if (photons[i].respawn_timer <= 0) reset_photon(i, i < NUM_ORBITING_PHOTONS);
            continue;
        }
        float current_x = photons[i].pos.x;
        float current_z = photons[i].pos.z;
        float y_dx1 = get_y_displacement(current_x + 0.1f, current_z);
        float y_dx2 = get_y_displacement(current_x - 0.1f, current_z);
        float slope_x = (y_dx1 - y_dx2) / 0.2f;
        float y_dz1 = get_y_displacement(current_x, current_z + 0.1f);
        float y_dz2 = get_y_displacement(current_x, current_z - 0.1f);
        float slope_z = (y_dz1 - y_dz2) / 0.2f;
        float force_magnitude = sqrt(slope_x * slope_x + slope_z * slope_z);
        if (force_magnitude > max_force) {
            slope_x = (slope_x / force_magnitude) * max_force;
            slope_z = (slope_z / force_magnitude) * max_force;
        }
        photons[i].vel.x -= slope_x * dt;
        photons[i].vel.z -= slope_z * dt;
        photons[i].pos.x += photons[i].vel.x * dt;
        photons[i].pos.z += photons[i].vel.z * dt;
        photons[i].pos.y = get_y_displacement(photons[i].pos.x, photons[i].pos.z);
        photons[i].history[photons[i].history_index] = photons[i].pos;
        photons[i].history_index = (photons[i].history_index + 1) % TRAIL_LENGTH;
        float dist_from_center = sqrt(photons[i].pos.x * photons[i].pos.x + photons[i].pos.z * photons[i].pos.z);
        if (i >= NUM_ORBITING_PHOTONS && (dist_from_center < 1.5f || dist_from_center > GRID_SIZE * 1.5f)) {
            photons[i].active = 0;
            photons[i].respawn_timer = (float)(rand() % 100) / 100.0f;
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    float cam_x = cam_dist * sin(cam_angle_z * M_PI / 180.0) * cos(cam_angle_x * M_PI / 180.0);
    float cam_y = cam_dist * sin(cam_angle_x * M_PI / 180.0);
    float cam_z = cam_dist * cos(cam_angle_z * M_PI / 180.0) * cos(cam_angle_x * M_PI / 180.0);
    gluLookAt(cam_x, cam_y, cam_z, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    glPointSize(1.0);
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_STARS; i++) glVertex3f(stars[i].x, stars[i].y, stars[i].z);
    glEnd();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glColor4f(0.0, 0.2, 0.4, 0.5);
    glLineWidth(1.0f);
    // Grid drawing loops would go here...

    glDisable(GL_BLEND);
    glColor3f(0.0, 0.0, 0.0);
    glutSolidSphere(1.5, 20, 20);
    glEnable(GL_BLEND);

    glDepthMask(GL_FALSE);
    for (int i = 0; i < NUM_PHOTONS; i++) {
        if (photons[i].active) {
            glBegin(GL_LINE_STRIP);
            for(int j = 0; j < TRAIL_LENGTH; j++) {
                int index = (photons[i].history_index + j) % TRAIL_LENGTH;
                float alpha = 0.7f * ((float)j / TRAIL_LENGTH);
                glColor4f(1.0, 1.0, 0.0, alpha);
                glVertex3fv((float*)&photons[i].history[index]);
            }
            glEnd();
        }
    }
    glDepthMask(GL_TRUE);

    for (int i = 0; i < NUM_PHOTONS; i++) {
        if (photons[i].active) {
            glPointSize(3.0);
            glColor3f(1.0, 1.0, 1.0);
            glBegin(GL_POINTS);
            glVertex3fv((float*)&photons[i].pos);
            glEnd();
        }
    }
    glDisable(GL_BLEND);
    glutSwapBuffers();
}

void mouseFunc(int button, int state, int x, int y) { /* Mouse logic */ }
void motionFunc(int x, int y) { /* Drag logic */ }

void init() {
    srand(time(NULL));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = (float)(rand() % 400 - 200);
        stars[i].y = (float)(rand() % 400 - 200);
        stars[i].z = (float)(rand() % 400 - 200);
    }
    for (int i = 0; i < NUM_PHOTONS; i++) {
        if (i < NUM_ORBITING_PHOTONS) reset_photon(i, 1);
        else {
            photons[i].active = 0;
            photons[i].respawn_timer = (float)(rand() % 300) / 100.0f;
        }
    }
    glClearColor(0.01f, 0.0f, 0.02f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 1.0, 1.0, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("CPU Black Hole Simulation");
    init();
    glutDisplayFunc(display);
    glutMouseFunc(mouseFunc);
    glutMotionFunc(motionFunc);
    glutTimerFunc(16, update, 0);
    glutMainLoop();
    return 0;
}
