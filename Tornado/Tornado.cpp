#include <GL/freeglut.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <fstream>

// ---Camera variables---
float camX = 1.0f, camY = 0.0f, camZ = 5.0f;
float targetX = 0.0f, targetY = 0.0f, targetZ = 0.0f;
float scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;

bool isPanning = false;      // Is the camera currently moving?
float panProgress = 0.0f;    // 0.0 = Start (Tornado), 1.0 = End (House)
float panSpeed = 0.005f;     // How fast the camera moves

// SCENE_TORNADO camera positions
float tornadoCamX = 0.0f, tornadoCamY = 15.0f, tornadoCamZ = 25.0f;
float tornadoTargetX = 0.0f, tornadoTargetY = 2.0f, tornadoTargetZ = 0.0f;

// SCENE_HOUSE camera positions
float houseCamX = -30.0f, houseCamY = 1.0f, houseCamZ = 20.0f;
float houseTargetX = 40.0f, houseTargetY = 2.0f, houseTargetZ = 0.0f;

// SCENE_TORNADO_CHASE camera positions
float chaseOffsetX = 3.0f, chaseOffsetY = 15.0f, chaseOffsetZ = 25.0f;

enum SceneState {
    SCENE_TORNADO,
    SCENE_HOUSE,
    SCENE_TORNADO_CHASE
};

SceneState currentScene = SCENE_TORNADO;

// ---Tornado world position variables---
float tornadoPosX = 0.0f;
float tornadoPosY = 0.0f;
float tornadoPosZ = 0.0f;
float tornadoSpeed = 2.0f;
bool tornadoActive = false;

// ---House world position variables---
float housePosX = -25.0f;
float housePosY = 1.0f;
float housePosZ = 20.0f;

// ---Collision variables---
float houseRadius = 4.0f;
float tornadoRadius = 3.0f;
bool houseDestroyed = false;

float globalTime = 0.0f;
const float dt = 0.016f; // timestep is 60 fps = 1/60 = 0.016

class RandomNumberEngine {
private:
    std::random_device rd;
    std::mt19937 gen;

public:
    RandomNumberEngine() : gen(rd()){}

    float range(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

    float angle() {
        return range(0.0f, 6.28318f); // 0 to 2pi
    }
};

class StarField {
private:
    struct Star {
        float x, y, z;
    };

    std::vector<Star> stars;
    int count;
    float width, height, depth;

    RandomNumberEngine rng;

    void generate() {
        stars.clear();
        stars.reserve(count);

        for (int i = 0; i < count; i++) {
            float x = rng.range(-0.5f, 0.5f) * width;                       // gives range(-width/2, +width/2) because of rng.range(-0.5f, 0.5f)
            float y = (0.7f + rng.range(-0.5f, 0.5f)) * (height - 0.7f);    // 0.7f lifts the points up in y coordinate to ensure that no stars overlap the ground
            float z = rng.range(-0.5f, 0.5f) * depth;
            stars.push_back({ x, y, z });
        }
    }

public:
    StarField(int count, float width, float height, float depth)
        : count(count), width(width), height(height), depth(depth) {
        generate();
    }

    void draw() {
        glPointSize(1.0f);
        glBegin(GL_POINTS);
        glColor3f(1, 1, 1);
        for (const auto& s : stars)
            glVertex3f(s.x, s.y, s.z);
        glEnd();
    }
};

class Tornado {
private:
    struct Grain {
        float x, y, z, angle;
    };

    std::vector<Grain> grains;
    const int maxParticles = 4000;

    const float height = 15.0f,     // limit to the height the tornado can reach      
                swayAmount = 1.0f,  // how much the tornado sways left/right
                swaySpeed = 0.5f;   // how fast it sways

    RandomNumberEngine rng;

    void tornadoCenter(float y, float& cx, float& cz) {
        float baseX = sinf(y * 0.3f) * 0.5f;            // y*0.3 controls the frequency of the wave along the y axis | sin() for smooth oscillation along the x axis
        float baseZ = cosf(y * 0.3f) * 0.5f;            // cos() for smooth oscillation along the z axis

        // entire tornado movement using sinusoidal motion
        cx = baseX + sinf(y * 0.2f + globalTime * swaySpeed) * swayAmount;              // y*0.2f is same for baseX | globalTime*swaySpeed is time dependent, it allows it to move continously over time
        cz = baseZ + cosf(y * 0.2f + 3.1415f / 2 + globalTime * swaySpeed) * swayAmount;// use of cos for phase shifted motion | pi/2 ensures that x and z sway isnt perfectly circular, it looks more erattic
    }

    // returns the radius of the funnel shape at a given height
    float funnelRadius(float y) {
        return 0.2f + (y / height) * 2.0f;  // y/height to normalize (only 0-1 range) | 2.0 scales it to the maximum radius of 2 | 0.2 is the minimum radius
    }

    void spawn() {
        if (grains.size() < maxParticles) {
            Grain g;

            g.x = g.y = g.z = 0.0f;
            g.angle = rng.angle();
            grains.push_back(g);
        }
    }

public:
    Tornado() { 
        grains.reserve(maxParticles);
    }

    void update() {
        spawn();

        for (auto& g : grains) {
            float cx, cz;
            tornadoCenter(g.y, cx, cz);

            g.angle += 1.0f;                                     // speed in which the particle rise

            float r = funnelRadius(g.y);                         // set distance from center line based on the y axis
            g.x = cx + cosf(g.angle) * r;                        // convert polar coordinates to cartesian (x = center + cos(angle) * radius)
            g.z = cz + sinf(g.angle) * r;

            g.y += 0.03f + rng.range(0.0f, 0.005f) * 0.005f;     // raise grain at @ y value with some randomness

            if (g.y > height) {                                  // reset to y = 0
                g.y = 0;
                g.angle = rng.angle();
            }
        }
    }

    void draw() {
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        for (auto& g : grains) {
            float t = g.y / height;                                                 //normalized height 0.0 to 1.0 | this will dictate the color based on the height of the point (gradient) 
            glColor3f(0.8f - (0.3f * t), 0.7f - (0.3f * t), 0.5f - (0.2f * t));     //at bottom (t=0) the color is 0.8,0.7,0.5 | the higher it gets the higher the "t" the darker the shade
            glVertex3f(g.x, g.y, g.z);                                  
        }
        glEnd();
    }
};

// --House--
// Set Texture
GLuint texWall;
GLuint texDoor;
GLuint texWindow;

GLuint loadBMP(const char* filename) {
    FILE* f;
    fopen_s(&f, filename, "rb");
    if (!f) return 0;

    unsigned char header[54];
    fread(header, 1, 54, f);

    int width = *(int*)&header[18];
    int height = *(int*)&header[22];

    int imageSize = 3 * width * height;
    unsigned char* data = new unsigned char[imageSize];
    fread(data, 1, imageSize, f);
    fclose(f);

    // Create texture
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
        0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    delete[] data;
    return texID;
}

// Cube for House
void texturedCube(float size, GLuint texture, float repeat = 1.0f) {
    float s = size / 2;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);

    // Front
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(repeat, 0); glVertex3f(s, -s, s);
    glTexCoord2f(repeat, repeat); glVertex3f(s, s, s);
    glTexCoord2f(0, repeat); glVertex3f(-s, s, s);

    // Back
    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);

    // Left
    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);

    // Right
    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(s, s, -s);

    // Top
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-s, s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, s);

    // Bottom
    glNormal3f(0, -1, 0);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, -s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, -s, s);

    glEnd();

    glDisable(GL_TEXTURE_2D);
}

// Roof
void texturedRoof(float width, float height, float depth, GLuint texture, float repeat = 1.0f) {
    float w = width / 2;
    float d = depth / 2;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glNormal3f(-1.0f, height, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-w, 0, -d);
    glTexCoord2f(repeat, 0.0f); glVertex3f(-w, 0, d);
    glTexCoord2f(repeat, repeat); glVertex3f(0, height, d);
    glTexCoord2f(0.0f, repeat); glVertex3f(0, height, -d);
    glEnd();

    // Right slope
    glBegin(GL_QUADS);
    glNormal3f(1.0f, height, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(w, 0, -d);
    glTexCoord2f(repeat, 0.0f); glVertex3f(w, 0, d);
    glTexCoord2f(repeat, repeat); glVertex3f(0, height, d);
    glTexCoord2f(0.0f, repeat); glVertex3f(0, height, -d);
    glEnd();

    // Front triangle
    glBegin(GL_TRIANGLES);
    glNormal3f(0.0f, height, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-w, 0, d);
    glTexCoord2f(repeat, 0.0f); glVertex3f(w, 0, d);
    glTexCoord2f(repeat / 2, repeat); glVertex3f(0, height, d); // peak
    glEnd();

    // Back triangle
    glBegin(GL_TRIANGLES);
    glNormal3f(0.0f, height, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-w, 0, -d);
    glTexCoord2f(repeat, 0.0f); glVertex3f(w, 0, -d);
    glTexCoord2f(repeat / 2, repeat); glVertex3f(0, height, -d); // peak
    glEnd();

    glDisable(GL_TEXTURE_2D);

}

// 3D Box
void house() {
    glMatrixMode(GL_MODELVIEW);

    glScalef(scaleX, scaleY, scaleZ);

    glPushMatrix();
    texturedCube(2, texWall); // House Base

    glTranslatef(0, 1, 0);
    glPushMatrix();                     // Roof
    texturedRoof(2.50, 1, 2, texDoor);  // <- please change this it sucks ass
    glPopMatrix();

    glPushMatrix();						// Door
    glTranslatef(0.525, -1.275, 1);
    glScalef(1.2, 2.75, 0.1);
    texturedCube(.5, texDoor, 2.0f);
    glPopMatrix();

    glPushMatrix();					    // Window
    glTranslatef(-.415, -0.85, 1);
    glScalef(1.25, 1.25, 0.01);
    texturedCube(.5, texWindow, 1.0f);  // This texture sucks ass
    glPopMatrix();

    glPopMatrix();

    glFlush();
}

// --Display Functions--
void drawGround() {
    glColor3f(0.45f, 0.35f, 0.25f);

    glBegin(GL_QUADS);
    glVertex3f(-100, 0, -50);
    glVertex3f(100, 0, -50);
    glVertex3f(100, 0, 50);
    glVertex3f(-100, 0, 50);
    glEnd();
}

StarField starField(200, 100.0f, 60.0f, 100.0f);
Tornado tornado;

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clears color and depth values of previous frame
    glLoadIdentity();
    
    gluLookAt(      
        camX, camY, camZ,             // camera position
        targetX, targetY, targetZ,    // look-at target point
        0, 1, 0);                     // up direction
    
    drawGround();
    starField.draw();

    glPushMatrix();
    glTranslatef(tornadoPosX, 0.0f, tornadoPosZ);
    tornado.draw();
    glPopMatrix();

    if (!houseDestroyed) {
        glPushMatrix();
        glTranslatef(-25.0f, 1.0f, 20.0f);
        glRotatef(-100.0f, 0.0f, 1.0f, 0.0f);
        house();
        glPopMatrix();
    }

    glutSwapBuffers();
}

void timer(int) {
    globalTime += dt;
    tornado.update();

	// ---Activate tornado movement---
    if (currentScene == SCENE_TORNADO_CHASE)
		tornadoActive = true;

    // ---Moves the tornado toward the house---
    if (tornadoActive) {
        float dx = housePosX - tornadoPosX;
        float dz = housePosZ - tornadoPosZ;

        float dist = sqrt(pow(dt, 2.0) + pow(dz, 2.0));

        if (dist > 0.001f) {
            dx /= dist;
            dz /= dist;

            tornadoPosX += dx * tornadoSpeed * dt;
            tornadoPosZ += dz * tornadoSpeed * dt;
        }

        // ---Check for collision---
        if (!houseDestroyed && dist < houseRadius + tornadoRadius)
            houseDestroyed = true;
    }

	// ---Camera Panning Logic | State machine---
    if (currentScene == SCENE_HOUSE) {
        panProgress += panSpeed;
        if (panProgress > 1.0f)
            panProgress = 1.0f;

        // Interpolate Position to house
        camX = tornadoCamX + (houseCamX - tornadoCamX) * panProgress;
        camY = tornadoCamY + (houseCamY - tornadoCamY) * panProgress;
        camZ = tornadoCamZ + (houseCamZ - tornadoCamZ) * panProgress;

        targetX = tornadoTargetX + (houseTargetX - tornadoTargetX) * panProgress;
        targetY = tornadoTargetY + (houseTargetY - tornadoTargetY) * panProgress;
        targetZ = tornadoTargetZ + (houseTargetZ - tornadoTargetZ) * panProgress;
    }
    else if (currentScene == SCENE_TORNADO_CHASE) {
        // camera follows tornado position
		camX = tornadoPosX - chaseOffsetX; 
        camY = tornadoPosY + chaseOffsetY;
        camZ = tornadoPosZ - chaseOffsetZ;

        // look at midpoint between tornado and house
		targetX = (tornadoPosX + housePosX) * 0.5f; 
        targetY = 2.0f;
        targetZ = (tornadoPosZ + housePosZ) * 0.5f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void lighting() {
    GLfloat lightpos[] = { 0.0, 0.0, 15.0 };
    GLfloat lightcolor[] = { 1.0, 1.0, 0.0 };
    GLfloat ambcolor[] = { 0.0, 0.0, 1.0 };

    glEnable(GL_LIGHTING);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambcolor);

    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightcolor);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightcolor);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightcolor);
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    texWall = loadBMP("wall.bmp");
    texDoor = loadBMP("door.bmp");
    texWindow = loadBMP("window.bmp");
    //Debug Texture
        /*if (!texWall) {
            printf("FAILED TO LOAD wall.bmp\n");
        }
        else {
            printf("Loaded wall.bmp successfully!\n");
        }*/
    lighting();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == ' ') { // Press Spacebar to change scenes
        if (currentScene == SCENE_TORNADO) {
            currentScene = SCENE_HOUSE;
			panProgress = 0.0f;
        }
        else if (currentScene == SCENE_HOUSE) {
            currentScene = SCENE_TORNADO_CHASE;
        }
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);   // double buffering | rgb color model | enables 3d depth (z buffer)
    glutInitWindowSize(1200, 720);
    glutCreateWindow("Tornado");
    
    glEnable(GL_DEPTH_TEST);                                    // objects hide behind others
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    
    //camera stuff
    glMatrixMode(GL_PROJECTION);                                
    glLoadIdentity();
    gluPerspective(60, 1200.0 / 720.0, 1, 100);                 // 60 degs (camera fov(vertical) | 1200720 aspect ratio of window | ` near clipping plane (dont draw things up close) | 100 far clipping plane (dont draw things too far))
    glMatrixMode(GL_MODELVIEW);

    camX = tornadoCamX; camY = tornadoCamY; camZ = tornadoCamZ;
    targetX = tornadoTargetX; targetY = tornadoTargetY; targetZ = tornadoTargetZ;

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, timer, 0);
    init();
    glutMainLoop();
}
