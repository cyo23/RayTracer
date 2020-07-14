/*==================================================================================
* COSC 363  Computer Graphics (2020)
* Department of Computer Science and Software Engineering, University of Canterbury.
*
* A basic ray tracer
* See Lab07.pdf, Lab08.pdf for details.
*===================================================================================
*/
#include <iostream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include "Sphere.h"
#include "SceneObject.h"
#include "Ray.h"
#include <GL/freeglut.h>
#include "Plane.h"
#include "TextureBMP.h"
#include "Cylinder.h"
#include "Cone.h"
using namespace std;

const float WIDTH = 20.0;  
const float HEIGHT = 20.0;
const float EDIST = 40.0;
const int NUMDIV = 500;
const int MAX_STEPS = 5;
const float XMIN = -WIDTH * 0.5;
const float XMAX =  WIDTH * 0.5;
const float YMIN = -HEIGHT * 0.5;
const float YMAX =  HEIGHT * 0.5;

vector<SceneObject*> sceneObjects;
TextureBMP texture;

//---The most important function in a ray tracer! ---------------------------------- 
//   Computes the colour value obtained by tracing a ray and finding its 
//     closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step)
{
    glm::vec3 backgroundCol(0);						//Background colour = (0,0,0)
    glm::vec3 lightPos(10, 40, -3);					//Light's position
	glm::vec3 color(0);
	SceneObject* obj;

    ray.closestPt(sceneObjects);					//Compare the ray with all objects in the scene
    if(ray.index == -1) return backgroundCol;		//no intersection
	obj = sceneObjects[ray.index];					//object on which the closest point of intersection is found

    if (ray.index == 7) {
        float texcoords = (ray.hit.x + 50) / 100;
        float texcoordt = (ray.hit.y + 15) / 80;
        if (texcoords > 0 && texcoords < 1 &&
            texcoordt > 0 && texcoordt < 1) {
            color = texture.getColorAt(texcoords, texcoordt);
            obj->setColor(color);
        }
    }


    if (ray.index == 4) {
        if ((int(ray.hit.y+ray.hit.z-4)) % 2 == 1 || int(ray.hit.y+ray.hit.z-4)%2 == -1) {
        color = glm::vec3(1, 1, 1);
        }
     else {color = glm::vec3(0.5, 0.5, 0);
        }
        obj->setColor(color);
 }

    color = obj->lighting(lightPos, -ray.dir, ray.hit);

    glm::vec3 lightVec = lightPos - ray.hit;
    Ray shadowRay(ray.hit, lightVec);
    shadowRay.closestPt(sceneObjects);


    if (obj->isReflective() && step < MAX_STEPS) {
        float rho = obj->getReflectionCoeff();
        glm::vec3 normalVec = obj->normal(ray.hit);
        glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
        Ray reflectedRay(ray.hit, reflectedDir);
        glm::vec3 reflectedColor = trace(reflectedRay, step + 1);
        color = color + (rho * reflectedColor);
    }

    else if (obj->isRefractive() && step < MAX_STEPS) {
        float refrCo = obj->getRefractionCoeff();
        glm::vec3 normal = obj->normal(ray.hit);
        glm::vec3 internalDir = glm::refract(ray.dir, normal, 1 / obj->getRefractiveIndex());
        Ray refrRay(ray.hit, internalDir);
        refrRay.closestPt(sceneObjects);
        glm::vec3 secondNormal = obj->normal(refrRay.hit);
        glm::vec3 exitDir = glm::refract(refrRay.hit, -secondNormal, obj->getRefractiveIndex());
        Ray exitRay(refrRay.hit, exitDir);
        glm::vec3 refractColor = trace(exitRay, step + 1);
        color = (refrCo * refractColor);
    }
    else if (obj->isTransparent() && step < MAX_STEPS) {
        float transCo = obj->getTransparencyCoeff();
        Ray internalRay(ray.hit, ray.dir);
        internalRay.closestPt(sceneObjects);
        glm::vec3 newColor = trace(internalRay, step + 1);
        color = (color * 0.3f) + (transCo * newColor);
    }
    if ((shadowRay.index > -1) && (shadowRay.dist < (glm::length(lightVec)))) {
        if (obj->isTransparent() || obj->isRefractive()){
            color = 0.99f * color;
        }
        else {

            color = 0.5f * color;
        }
    }


	return color;
}

glm::vec3 anti_alias(float xp, float yp, float cellX, float cellY, glm::vec3 eye, int step) {


    //list<glm::vec3> colors;
    glm::vec3 colors[4];
    for (int i = 0; i < 4; i++) {
        glm::vec3 newDir(xp + 0.25 * cellX, yp + 0.25 * cellY, -EDIST);
        if (i == 1) {
            glm::vec3 newDir(xp + 0.25 * cellX, yp + 0.75 * cellY, -EDIST);
        }
        else if (i == 2) {
            glm::vec3 newDir(xp + 0.75 * cellX, yp + 0.25 * cellY, -EDIST);
        }
        else if (i == 3){
            glm::vec3 newDir(xp + 0.75 * cellX, yp + 0.75 * cellY, -EDIST);
        }
        Ray ray = Ray(eye, newDir);
        glm::vec3 newCol = trace(ray, 1);
        colors[i] = newCol;
    }

    if (step < 3) {

        float averageR = (colors[0].r + colors[1].r + colors[2].r + colors[3].r) / 4;
        float averageG = (colors[0].g + colors[1].g + colors[2].g + colors[3].g) / 4;
        float averageB = (colors[0].b + colors[1].b + colors[2].b + colors[3].b) / 4;
        glm::vec3 averages(averageR, averageG, averageB);

        for (int i = 0; i < 4; i++) {
            float rmax = colors[i].r > averages.r ? colors[i].r : averages.r;
            float rmin = colors[i].r < averages.r ? colors[i].r : averages.r;
            float gmax = colors[i].g > averages.g ? colors[i].g : averages.g;
            float gmin = colors[i].g < averages.g ? colors[i].g : averages.g;
            float bmax = colors[i].b > averages.b ? colors[i].b : averages.b;
            float bmin = colors[i].b < averages.b ? colors[i].b : averages.b;
            float difference = ((rmax - rmin) + (gmax - gmin) + (bmax - bmin)); //compute Manhatten distance for deviation from average

            if (difference > 0.5) {
                if (i == 0) {
                    colors[i] = anti_alias(xp, yp, 0.25 * cellX, 0.25 * cellY, eye, step + 1);
                }
                else if (i == 1) {
                    colors[i] = anti_alias(xp, yp, 0.25 * cellX, 0.75 * cellY, eye, step + 1);
                }
                else if (i == 2) {
                    colors[i] = anti_alias(xp, yp, 0.75 * cellX, 0.25 * cellY, eye, step + 1);
                }
                else {
                    colors[i] = anti_alias(xp, yp, 0.75 * cellX, 0.75 * cellY, eye, step + 1);
                }
            }

        }
    }

    float finalr = (colors[0].r + colors[1].r + colors[2].r + colors[3].r) / 4;
    float finalg = (colors[0].g + colors[1].g + colors[2].g + colors[3].g) / 4;
    float finalb = (colors[0].b + colors[1].b + colors[2].b + colors[3].b) / 4;
    glm::vec3 final_averages(finalr, finalg, finalb);

    return final_averages;
}

//---The main display module -----------------------------------------------------------
// In a ray tracing application, it just displays the ray traced image by drawing
// each cell as a quad.
//---------------------------------------------------------------------------------------
void display()
{
	float xp, yp;  //grid point
	float cellX = (XMAX-XMIN)/NUMDIV;  //cell width
	float cellY = (YMAX-YMIN)/NUMDIV;  //cell height
	glm::vec3 eye(0., 0., 0.);

	glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glBegin(GL_QUADS);  //Each cell is a tiny quad.

	for(int i = 0; i < NUMDIV; i++)	//Scan every cell of the image plane
	{
		xp = XMIN + i*cellX;
		for(int j = 0; j < NUMDIV; j++)
		{
			yp = YMIN + j*cellY;
            glm::vec3 color = anti_alias(xp, yp, cellX, cellY, eye, 1);

            glColor3f(color.r, color.g, color.b);
			glVertex2f(xp, yp);				//Draw each cell with its color value
			glVertex2f(xp+cellX, yp);
			glVertex2f(xp+cellX, yp+cellY);
			glVertex2f(xp, yp+cellY);
        }
    }
    glEnd();
    glFlush();
}

void init_planes() {

    Plane* floorPlane = new Plane(glm::vec3(-50., -15, 0), //bot left
        glm::vec3(50., -15, 0), //bot right
        glm::vec3(50., -15, -200), //top right
        glm::vec3(-50., -15, -200)); //top left
    floorPlane->setSpecularity(false);
    sceneObjects.push_back(floorPlane);//index7

    Plane* backPlane = new Plane(glm::vec3(50, -15, -200), //bot right
        glm::vec3(50, 65, -200), // top right
        glm::vec3(-50, 65, -200), //top left
        glm::vec3(-50, -15, -200)); //bot left
    backPlane->setColor(glm::vec3(1, 1, 1));
    backPlane->setSpecularity(false);
    sceneObjects.push_back(backPlane);

    Plane* topPlane = new Plane(glm::vec3(-50., 65, -200), //reversed floor plane raised 85
        glm::vec3(50., 65, -200),
        glm::vec3(50., 65, 0),
        glm::vec3(-50., 65, 0));
    topPlane->setColor(glm::vec3(1, 1, 0));
    topPlane->setSpecularity(false);
    sceneObjects.push_back(topPlane);

    Plane* frontPlane = new Plane(glm::vec3(-50, -15, 0), //reversed backplane at z == 0
        glm::vec3(-50, 65, 0),
        glm::vec3(50, 65, 0),
        glm::vec3(50, -15, 0));
    frontPlane->setColor(glm::vec3(1, 1, 0));
    frontPlane->setSpecularity(false);
    sceneObjects.push_back(frontPlane);

    Plane* leftPlane = new Plane(glm::vec3(-50, -15, 0), //bot left
        glm::vec3(-50, -15, -200), //bot right
        glm::vec3(-50, 65, -200), //top right
        glm::vec3(-50, 65, 0)); //top left
    leftPlane->setColor(glm::vec3(1, 1, 0));
    leftPlane->setSpecularity(false);
    sceneObjects.push_back(leftPlane);

    Plane* rightPlane = new Plane(glm::vec3(50, 65, 0), //reversed left plane where moved 100 along x
        glm::vec3(50, 65, -200),
        glm::vec3(50, -15, -200),
        glm::vec3(50, -15, 0));
    rightPlane->setColor(glm::vec3(1, 1, 0));
    rightPlane->setSpecularity(false);
    sceneObjects.push_back(rightPlane);

    Plane* cubeFront = new Plane(glm::vec3(15, -15, -70), //bot right
        glm::vec3(15, -10, -70), //top right
        glm::vec3(10, -10, -70), // top left
        glm::vec3(10, -15, -70));// bot left
    cubeFront->setColor(glm::vec3(1, 1, 1));
    sceneObjects.push_back(cubeFront);

    Plane* cubeTop = new Plane(glm::vec3(15, -10, -70), //bot right
        glm::vec3(15, -10, -75), //top right
        glm::vec3(10, -10, -75), //top left
        glm::vec3(10, -10, -70)); //bot left
    cubeTop->setColor(glm::vec3(1, 1, 1));
    sceneObjects.push_back(cubeTop);

    Plane* cubeBack = new Plane(glm::vec3(10, -15, -75), //front piece moved back 5
        glm::vec3(10, -10, -75),
        glm::vec3(15, -10, -75),
        glm::vec3(15, -15, -75));
    cubeBack->setColor(glm::vec3(0.2, 0.5, 0.5));
    sceneObjects.push_back(cubeBack);

    Plane* cubeLeft = new Plane(glm::vec3(10, -15, -70),
        glm::vec3(10, -10, -70),
        glm::vec3(10, -10, -75),
        glm::vec3(10, -15, -75));
    cubeLeft->setColor(glm::vec3(1, 1, 1));
    sceneObjects.push_back(cubeLeft);
}
void initialize()
{
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(XMIN, XMAX, YMIN, YMAX);
    texture = TextureBMP("timg.bmp");

    glClearColor(0, 0, 0, 1);

    Sphere *sphere1 = new Sphere(glm::vec3(-5.0, 0.0, -90.0), 15.0);
    sphere1->setReflectivity(true, 0.8);
    sphere1->setColor(glm::vec3(0, 0, 1));   //Set colour to blue
    sceneObjects.push_back(sphere1);		 //Add sphere to scene objects

    Sphere *sphere2 = new Sphere(glm::vec3(5.0, 5.0, -70.0), 5.0);
    sphere2->setColor(glm::vec3(1, 0, 0));
    sphere2->setTransparency(true, 0.8f);
    sceneObjects.push_back(sphere2);

    Sphere *sphere3 = new Sphere(glm::vec3(-3.0, -10.0, -60.0), 5.0);
    sphere3->setColor(glm::vec3(1, 1, 0));
    sphere3->setRefractivity(true, 1.0f, 1.2);
    sceneObjects.push_back(sphere3);

    Sphere *sphere4 = new Sphere(glm::vec3(10.0, 10.0, -60.0), 3.0);
    sphere4->setColor(glm::vec3(0, 1, 1));
    sceneObjects.push_back(sphere4);//index3

    Cylinder *cylinder = new Cylinder(glm::vec3(6, -15, -70), 2, 8.0, glm::vec3(1, 0, 0));
    sceneObjects.push_back(cylinder);

    Cone *cone = new Cone(glm::vec3(-14, -15.0, -70.0), 3, 12.0, glm::vec3(1, 0.7, 0.7));
    sceneObjects.push_back(cone);//index5

    init_planes();
}


int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Raytracing");

    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
