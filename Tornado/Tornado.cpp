#include <GL/freeglut.h>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>

using namespace std;

float globalTime = 0.0f;
const float dt = 0.016f; //timestep is 60 fps = 1/60 = 0.016

class RandomNumberEngine {
private:
    random_device rd;
    mt19937 gen;

public:
    RandomNumberEngine() : gen(rd()){}

    float range(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

    float angle() {
        return range(0.0f, 6.28318f); //0 to 2pi
    }

};

class StarField {
private:
    struct Star {
        float x, y, z;
    };

    vector<Star> stars;
    int count;
    float width, height, depth;

    RandomNumberEngine rng;

    void generate() {
        stars.clear();
        stars.reserve(count);

        for (int i = 0; i < count; i++) {
            float x = rng.range(-0.5f, 0.5f) * width;                       //gives range(-width/2, +width/2) because of rng.range(-0.5f, 0.5f)
            float y = (0.7f + rng.range(-0.5f, 0.5f)) * (height - 0.7f);    //0.7f lifts the points up in y coordinate to ensure that no stars overlap the ground
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

    vector<Grain> grains;
    const int maxParticles = 4000;

    const float height = 10.0f,     //limit to the height the tornado can reach      
                swayAmount = 1.0f,  //how much the tornado sways left/right
                swaySpeed = 0.5f;   //how fast it sways

    RandomNumberEngine rng;

    void tornadoCenter(float y, float& cx, float& cz) {
        float baseX = sinf(y * 0.3f) * 0.5f;            //y*0.3 controls the frequency of the wave along the y axis | sin() for smooth oscillation along the x axis
        float baseZ = cosf(y * 0.3f) * 0.5f;            //cos() for smooth oscillation along the z axis

        //entire tornado movement using sinusoidal motion
        cx = baseX + sinf(y * 0.2f + globalTime * swaySpeed) * swayAmount;              //y*0.2f is same for baseX | globalTime*swaySpeed is time dependent, it allows it to move continously over time
        cz = baseZ + cosf(y * 0.2f + 3.1415f / 2 + globalTime * swaySpeed) * swayAmount;//use of cos for phase shifted motion | pi/2 ensures that x and z sway isnt perfectly circular, it looks more erattic
    }

    //returns the radius of the funnel shape at a given height
    float funnelRadius(float y) {
        return 0.2f + (y / height) * 2.0f;  //y/height to normalize (only 0-1 range) | 2.0 scales it to the maximum radius of 2 | 0.2 is the minimum radius
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

            g.angle += 1.0f;                            //speed in which the particle rise

            float r = funnelRadius(g.y);                //set distance from center line based on the y axis
            g.x = cx + cosf(g.angle) * r;               //convert polar coordinates to cartesian (x = center + cos(angle) * radius)
            g.z = cz + sinf(g.angle) * r;

            g.y += 0.03f + rng.range(0.0f, 0.005f) * 0.005f;     //raise grain at @ y value with some randomness

            if (g.y > height) {                         //reset to y = 0
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

void drawGround() {
    glColor3f(0.45f, 0.35f, 0.25f);

    glBegin(GL_QUADS);
    glVertex3f(-100, 0, -20);
    glVertex3f(100, 0, -20);
    glVertex3f(100, 0, 20);
    glVertex3f(-100, 0, 20);
    glEnd();
}

StarField starField(200, 40.0f, 25.0f, 40.0f);
Tornado tornado;

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clears color and depth values of previous frame
    glLoadIdentity();
    
    gluLookAt(      //position of camera
        0, 6, 15,   //camera position
        0, 2, 0,    //look-at target point
        0, 1, 0);   //up direction
    
    drawGround();
    starField.draw();
    tornado.draw();
    glutSwapBuffers();
}

void timer(int) {
    globalTime += dt;               //to track the time in the scene
    tornado.update();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);   //double buffering | rgb color model | enables 3d depth (z buffer)
    glutInitWindowSize(1200, 720);
    glutCreateWindow("Tornado");

    glEnable(GL_DEPTH_TEST);                                    //objects hide behind others
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    
    //camera stuff
    glMatrixMode(GL_PROJECTION);                                
    glLoadIdentity();
    gluPerspective(60, 1200.0 / 720.0, 1, 100);                 //60 degs (camera fov(vertical) | 1200720 aspect ratio of window | ` near clipping plane (dont draw things up close) | 100 far clipping plane (dont draw things too far))
    glMatrixMode(GL_MODELVIEW);

    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);
    glutMainLoop();
}
