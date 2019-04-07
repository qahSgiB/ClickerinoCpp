/* 
 *	Powershell command to run this:
 * 		g++ Clickerino.cpp -o a.exe -std=c++11 -lgdiplus -lgdi32 -lopengl32 -lglu32; .\a.exe
 * 
 *	ToDo:
 *		convert pairs to tuples
 *		responsive desgin
 *		definitions and declarations (now implemanted only for Demo)
 */

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#include <iostream>
#include <cmath>
#include <utility>
#include <vector>
#include <tuple>
#include <string>
#include <fstream>
#include <map>

#include <functional>

#include <ctime>

using namespace std;



/*
 * Lib: 3dEngine
 */

vector<string> split(string s, string delimiter) {
	vector<string> result;

	int start = 0;
	int end = s.find(delimiter);

	while (end != string::npos) {
		string substring = s.substr(start, end-start);
		result.push_back(substring);

		start = end+delimiter.length();
		end = s.find(delimiter, start);
	}

	string substring = s.substr(start, s.length());
	result.push_back(substring);

	return result;
}

void debug(int num) {
	cout << "debug (num: " << num << ")" << endl;
}



class vector3d {
	public:
		float x;
		float y;
		float z;

		vector3d() {}

		vector3d(float x, float y, float z): x(x), y(y), z(z) {}
};

class vector2d {
	public:
		float x;
		float y;

		vector2d() {}

		vector2d(float x, float y): x(x), y(y) {}
};

class Material {
	public:
		olc::Pixel color;
		string name;

		Material() {}

		Material(olc::Pixel color, string name): color(color), name(name) {}

		static map<string, Material> loadFromFile(string folderPath) {
			map<string, Material> materials;
			bool materialExists = false;
			olc::Pixel color;
			string name;

			ifstream file(folderPath+"/object.mtl");

			if (!(!file)) {
				for (string line; getline(file, line); ) {  
					if (line.length() > 0) {

						// is comment ?
						if (line.at(0) != '#') {
							vector<string> infos = split(line, " ");
							string infoType = infos[0];

							if (infoType == "newmtl") {
								if (materialExists) {
									materials.insert(make_pair(name, Material(color, name)));
								}

								name = infos[1];
								materialExists = true;
							} else if (infoType == "Kd") {
								int red = stof(infos[1])*255;
								int green = stof(infos[2])*255;
								int blue = stof(infos[3])*255;

								color = olc::Pixel(red, green, blue);
							}
						}
					}
				}

				file.close();

				materials.insert(make_pair(name, Material(color, name)));
			}

			return materials;
	}
};

class Object {
	public:
		vector<vector3d> points;
		vector<tuple<tuple<int, int, int>, Material>> triangles;

		vector3d pos;
		vector3d scale;
		vector3d rotation;

		Object() {};

		Object(vector<vector3d> points, vector<tuple<tuple<int, int, int>, Material>> triangles): points(points), triangles(triangles) {
			pos = {0, 0, 0};
			rotation = {0, 0, 0};
			scale = {1, 1, 1};
		}

		static Object copy(Object object) {
			vector<vector3d> points;
			vector<tuple<tuple<int, int, int>, Material>> triangles;

			for (vector<vector3d>::iterator point = object.points.begin(); point != object.points.end(); point++) {
				points.push_back(*point);
			}

			for (vector<tuple<tuple<int, int, int>, Material>>::iterator triangle = object.triangles.begin(); triangle != object.triangles.end(); triangle++) {
				triangles.push_back(*triangle);
			}

			return Object(points, triangles);
		}

		static Object loadFromFile(string folderPath) {
			vector<vector3d> points;
			vector<tuple<tuple<int, int, int>, Material>> triangles;
			string activeMaterial = "";
			Material defaultMaterial = Material(olc::Pixel(0, 0, 0), "default_material");

			map<string, Material> materials = Material::loadFromFile(folderPath);

			ifstream file(folderPath+"/object.obj");

			if (!(!file)) {
				for (string line; getline(file, line); ) {  
					if (line.length() > 0) {

						// is comment ?
						if (line.at(0) != '#') {
							vector<string> infos = split(line, " ");
							string infoType = infos[0];

							if (infoType == "v") {
								float x = stof(infos[1]);
								float y = stof(infos[3]);
								float z = stof(infos[2]);
								points.push_back({x, y, z});
							} else if (infoType == "f") {
								int a = stoi(infos[1])-1;
								int b = stoi(infos[2])-1;
								int c = stoi(infos[3])-1;
								tuple<int, int, int> xtriangle(a, b, c);

								Material material;
								if (activeMaterial == "") {
									material = defaultMaterial;
								} else {
									material = materials[activeMaterial];
								}

								tuple<tuple<int, int, int>, Material> triangle(xtriangle, material);
								triangles.push_back(triangle);
							} else if (infoType == "usemtl") {
								activeMaterial = infos[1];
							}
						}
					}
				}

				file.close();
			}

			return Object(points, triangles);
		}

		void setPos(vector3d newPos) {
			pos = newPos;
		}

		void setRotation(vector3d newRotation) {
			rotation = newRotation;
		}

		void setScale(vector3d newScale) {
			scale = newScale;
		}

		vector<vector3d> getPoints() {
			vector<vector3d> pointsx;

			for (vector<vector3d>::iterator point = points.begin(); point != points.end(); point++) {
				float x = point->x;
				float y = point->y;
				float z = point->z;

				float cosX = cos(rotation.x);
				float sinX = sin(rotation.x);
				y = y*cosX-z*sinX;
				z = y*sinX+z*cosX;

				float cosY = cos(rotation.y);
				float sinY = sin(rotation.y);
				z = z*cosY-x*sinY;
				x = z*sinY+x*cosY;

				float cosZ = cos(rotation.z);
				float sinZ = sin(rotation.z);
				x = x*cosZ-y*sinZ;
				y = x*sinZ+y*cosZ;

				pointsx.push_back({x*scale.x+pos.x, y*scale.y+pos.y, z*scale.z+pos.z});
			}

			return pointsx;
		}
};

class Camera {
	public:
		vector3d center;
		vector3d direction;
		vector3d view1;
		vector3d view2;

		Camera() {}

		Camera(vector3d center, vector3d direction, vector3d view1, vector3d view2): center(center), direction(direction), view1(view1), view2(view2) {}
};

class Engine {
	public:
		float tre;
		Camera camera;

		Engine() {
			tre = 0.0000005;
		}

		Engine(float tre): tre(tre) {}

		void setCamera(Camera _camera) {
			camera = _camera;
		}

		pair<vector2d, bool> calculatePoint(vector3d point) {
			pair<vector2d, bool> result;

			vector3d d = camera.direction;
			vector3d v = camera.view1;
			vector3d u = camera.view2;
			vector3d o = {point.x-camera.center.x, point.y-camera.center.y, point.z-camera.center.z};

			float det =  d.x*v.y*u.z+v.x*u.y*d.z+u.x*d.y*v.z-u.x*v.y*d.z-v.x*d.y*u.z-d.x*u.y*v.z;
			float aDet = o.x*v.y*u.z+v.x*u.y*o.z+u.x*o.y*v.z-u.x*v.y*o.z-v.x*o.y*u.z-o.x*u.y*v.z;
			float bDet = d.x*o.y*u.z+o.x*u.y*d.z+u.x*d.y*o.z-u.x*o.y*d.z-o.x*d.y*u.z-d.x*u.y*o.z;
			float cDet = d.x*v.y*o.z+v.x*o.y*d.z+o.x*d.y*v.z-o.x*v.y*d.z-v.x*d.y*o.z-d.x*o.y*v.z;

			float a = aDet/det;
			float b = bDet/det;
			float c = cDet/det;

			if (a <= 0) {
				result.second = false;

				return result;
			} else {
				float xMultiplier = (b/a+1.0)/2.0;
				float yMultiplier = (c/a+1.0)/2.0;

				result.second = true;
				result.first = {xMultiplier, yMultiplier};

				return result;
			}
		}

		vector<pair<vector2d, bool>> calculatePoints(vector<vector3d> points) {
			vector<pair<vector2d, bool>> points2d;

			for (vector<vector3d>::iterator point = points.begin(); point != points.end(); ++point) {
				points2d.push_back(calculatePoint(*point));
			}

			return points2d;
		}

		float calculateDistance(vector3d point) {
			vector3d center = camera.center;
			float distance = sqrt(powf(center.x-point.x, 2)+powf(center.y-point.y, 2)+powf(center.z-point.z, 2));

			return distance;
		}

		vector<float> calculateDistances(vector<vector3d> points) {
			vector<float> distances;

			for (vector<vector3d>::iterator point = points.begin(); point != points.end(); ++point) {
				distances.push_back(calculateDistance(*point));
			}

			return distances;
		}

		void renderObjects(vector<Object> objects, olc::PixelGameEngine pgengine) {
			int width = pgengine.ScreenWidth();
			int height = pgengine.ScreenHeight();
			float *depthBuffer = new float[width*height];

			for (int x = 0; x < width; x++) {
				for (int y = 0; y < height; y++) {
					depthBuffer[y*width+x] = -1;
				}
			}

			for (vector<Object>::iterator xobject = objects.begin(); xobject != objects.end(); xobject++) {
				Object object = *xobject;

				vector<vector3d> points3d = object.getPoints();
				vector<pair<vector2d, bool>> xpoints2d = calculatePoints(points3d);

				// for (vector<tuple<tuple<int, int, int>, Material>>::iterator triangle = object.triangles.begin(); triangle != object.triangles.end(); ++triangle) {
				for (int index = 0; index < object.triangles.size(); index++) {
					tuple<tuple<int, int, int>, Material> triangle = object.triangles[index];

					tuple<int, int, int> xtriangle = get<0>(triangle);
					int a = get<0>(xtriangle);
					int b = get<1>(xtriangle);
					int c = get<2>(xtriangle);

					pair<vector2d, bool> xpointA = xpoints2d[a];
					pair<vector2d, bool> xpointB = xpoints2d[b];
					pair<vector2d, bool> xpointC = xpoints2d[c];

					if (xpointA.second && xpointB.second && xpointC.second) {
						
						/* seting up variables */

						Material material = get<1>(triangle);
						olc::Pixel color = material.color;

						vector2d unspointA = xpointA.first;
						vector2d unspointB = xpointB.first;
						vector2d unspointC = xpointC.first;

						unspointA.x *= width;
						unspointB.x *= width;
						unspointC.x *= width;
						unspointA.y *= height;
						unspointB.y *= height;
						unspointC.y *= height;

						vector2d pointA;
						vector2d pointB;
						vector2d pointC;

						vector3d unspoint3dA = points3d[a];
						vector3d unspoint3dB = points3d[b];
						vector3d unspoint3dC = points3d[c];

						vector3d point3dA;
						vector3d point3dB;
						vector3d point3dC;

						/* sorting triangle points */

						if (unspointA.y <= unspointB.y && unspointA.y <= unspointC.y) {
							pointA = unspointA;
							point3dA = unspoint3dA;
							if (unspointB.y <= unspointC.y) {
								pointB = unspointB;
								point3dB = unspoint3dB;
								pointC = unspointC;
								point3dC = unspoint3dC;
							} else {
								pointB = unspointC;
								point3dB = unspoint3dC;
								pointC = unspointB;
								point3dC = unspoint3dB;
							}

						} else if (unspointB.y <= unspointA.y && unspointB.y <= unspointC.y) {
							pointA = unspointB;
							point3dA = unspoint3dB;
							if (unspointA.y <= unspointC.y) {
								pointB = unspointA;
								point3dB = unspoint3dA;
								pointC = unspointC;
								point3dC = unspoint3dC;
							} else {
								pointB = unspointC;
								point3dB = unspoint3dC;
								pointC = unspointA;
								point3dC = unspoint3dA;
							}

						} else if (unspointC.y <= unspointB.y && unspointC.y <= unspointA.y) {
							pointA = unspointC;
							point3dA = unspoint3dC;
							if (unspointB.y <= unspointA.y) {
								pointB = unspointB;
								point3dB = unspoint3dB;
								pointC = unspointA;
								point3dC = unspoint3dA;
							} else {
								pointB = unspointA;
								point3dB = unspoint3dA;
								pointC = unspointB;
								point3dC = unspoint3dB;
							}
						}

						/* seting up variables */

						int pointAx = (int) pointA.x;
						int pointAy = (int) pointA.y;
						int pointBx = (int) pointB.x;
						int pointBy = (int) pointB.y;
						int pointCx = (int) pointC.x;
						int pointCy = (int) pointC.y;

						int l = pointCy-pointAy;
						int l1 = pointBy-pointAy;
						int l2 = pointCy-pointBy;
	
						float d = (pointAx-pointCx)/((float) (pointAy-pointCy));

						vector2d u = {(float) pointBx-pointAx, (float) pointBy-pointAy};
						vector2d v = {(float) pointCx-pointBx, (float) pointCy-pointBy};
						float det = u.x*v.y-v.x*u.y;

						/* drawing top half of triangle */

						if (l1 > 0) {
							float d1 = (pointAx-pointBx)/((float) (pointAy-pointBy));
							float x1 = pointAx;
							float x2 = pointAx;

							bool dIsRight = d1 < d;

							for (int y = pointAy; y <= pointBy; y++) {
								for (int x = (int) x1; x <= (int) x2; x++) {
									if (x >= 0 && x < width && y >= 0 && y < height) {
										float kDet = (x-pointAx)*v.y-v.x*(y-pointAy);
										float k = kDet/det;
										float lDet = u.x*(y-pointAy)-(x-pointAx)*u.y;
										float l = lDet/det;

										float point3dx = point3dA.x+k*(point3dB.x-point3dA.x)+l*(point3dC.x-point3dB.x);
										float point3dy = point3dA.y+k*(point3dB.y-point3dA.y)+l*(point3dC.y-point3dB.y);
										float point3dz = point3dA.z+k*(point3dB.z-point3dA.z)+l*(point3dC.z-point3dB.z);

										float pDistance = calculateDistance({point3dx, point3dy, point3dz});
									
										int depthBufferIndex = y*width+x;
										if (depthBuffer[depthBufferIndex] > pDistance || depthBuffer[depthBufferIndex] == -1) {
											pgengine.Draw(x, y, color);

											depthBuffer[depthBufferIndex] = pDistance;
										}
									}
								}

								if (dIsRight) {
									x1 += d1;
									x2 += d;
								} else {
									x1 += d;
									x2 += d1;
								}
							}
						}

						/* drawing bottom half of triangle */

						if (l2 > 0) {
							float d2 = (pointBx-pointCx)/((float) (pointBy-pointCy));
							float x1 = pointCx;
							float x2 = pointCx;

							bool dIsRight = d2 < d;

							for (int y = pointCy; y > pointBy; y--) {
								for (int x = (int) x1; x <= (int) x2; x++) {
									if (x >= 0 && x < width && y >= 0 && y < height) {
										float kDet = (x-pointAx)*v.y-v.x*(y-pointAy);
										float k = kDet/det;
										float lDet = u.x*(y-pointAy)-(x-pointAx)*u.y;
										float l = lDet/det;

										float point3dx = point3dA.x+k*(point3dB.x-point3dA.x)+l*(point3dC.x-point3dB.x);
										float point3dy = point3dA.y+k*(point3dB.y-point3dA.y)+l*(point3dC.y-point3dB.y);
										float point3dz = point3dA.z+k*(point3dB.z-point3dA.z)+l*(point3dC.z-point3dB.z);

										float pDistance = calculateDistance({point3dx, point3dy, point3dz});
									
										int depthBufferIndex = y*width+x;
										if (depthBuffer[depthBufferIndex] > pDistance || depthBuffer[depthBufferIndex] == -1) {
											pgengine.Draw(x, y, color);

											depthBuffer[depthBufferIndex] = pDistance;
										}
									}
								}

								if (!dIsRight) {
									x1 -= d2;
									x2 -= d;
								} else {
									x1 -= d;
									x2 -= d2;
								}
							}
						}
					}
				}
			}

			delete[] depthBuffer;
		}
};



/*
 * Lib: StateManager
 */

class State {
	public:
		string name;

		State() {}

		State(string name): name(name) {}

		virtual void onStart() {}

		virtual bool onUpdate(float elapsedTime) {}

		virtual void onEnd() {}
};

class StateManager {
	public:
		map<string, State*> states;
		string activeState;

		StateManager() {}

		void addState(State* state) {
			states.insert(make_pair(state->name, state));

			activeState = "";
		}

		bool update(float elapsedTime) {
			if (activeState != "") {
				return states[activeState]->onUpdate(elapsedTime);
			} else {
				return true;
			}
		}

		void setState(string newState) {
			if (activeState != "") {
				states[activeState]->onEnd();
			}
			activeState = newState;
			states[activeState]->onStart();
		}
};



/*
 * Main: ClickerinoCpp
 */

int randInt(int min, int max) {
	return rand()%(max-min+1)+min;
}

float randFloat(float min, float max, int decimalPlaces) {
	int mult = pow(10, decimalPlaces);
	return ((float) randInt(min*mult, max*mult))/mult;
}



class Block {
	public:
		float x;
		float y;

		float xVel;

		Object drawObject;
		vector3d rotation;

		Block() {}

		Block(float y, float velMultiplier, vector3d rotation): y(y), rotation(rotation) {
			x = 60;

			xVel = -17*velMultiplier;

			drawObject = Object::loadFromFile("Objects/block");
		}

		static Block spawn(float velMultiplier) {
			return Block(randInt(-11, 11), velMultiplier, {randInt(-3, 3)*90.0f, randInt(-3, 3)*90.0f, randInt(-3, 3)*90.0f});
		}

		void update(float elapsedTime) {
			x += xVel*elapsedTime;

			drawObject.setRotation({rotation.x*(3.14159f/2), rotation.y*(3.14159f/2), rotation.z*(3.14159f/2)});
			drawObject.setPos({x, y, 0});
			drawObject.setScale({1.5, 1.5, 1.5});
		}

		bool end() {
			return x < -12+5+1.5;
		}
};

class Bullet {
	public:
		float x;
		float y;
		float xVel;

		Object drawObject;

		Bullet() {}

		Bullet(float x, float y): x(x), y(y) {
			xVel = 35;

			drawObject = Object::loadFromFile("Objects/bullet");
		}

		void update(float elapsedTime) {
			x += xVel*elapsedTime;

			drawObject.setPos({x, y, 0.0});
		}

		bool end() {
			return x > 60;
		}

		bool collide(Block block) {
			float blockMin = block.x-1.5;
			float blockMax = block.x+1.5;

			float xMin = x-0.5;
			float xMax = x+0.5;

			return xMin <= blockMax && xMax >= blockMin && abs(block.y-y) < 1.5+0.2;
		}
};

class Laser {
	public:
		float x;
		float y;
		float xVel;

		Object drawObject;

		Laser() {}

		Laser(float x, float y): x(x), y(y) {
			xVel = 70;

			drawObject = Object::loadFromFile("Objects/laser");
		}

		void update(float elapsedTime) {
			x += xVel*elapsedTime;

			drawObject.setPos({x, y, 0});
		}

		bool end() {
			return x > 60;
		}
};

class Player {
	public:
		float y;
		float yVel;
		float maxYVel;

		float yAcc;
		float accDir;
		float accN;

		float reloading;
		float reloadTime;

		int bullets;

		float laserTime;
		float laserTimeMax;
		float laserReload;
		float laserReloadTime;

		int health;

		int score;

		Object drawObject;

		Player() {
			y = 0;
			yVel = 0;
			maxYVel = 35;

			yAcc = 50;
			accDir = 0;

			reloading = 0;
			reloadTime = 0.5;

			bullets = 8; 

			laserTime = 0;
			laserTimeMax = 2.5;
			laserReload = 30;

			health = 3;

			score = 0;

			drawObject = Object::loadFromFile("Objects/spaceShip");
		}

		void update(float elapsedTime) {
			yVel += accDir*yAcc*elapsedTime;
			if (yVel > maxYVel) {
				yVel = maxYVel;
			}
			if (yVel < -maxYVel) {
				yVel = -maxYVel;
			}

			if (accDir == 0) {
				if (yVel > 0) {
					yVel += -yAcc*elapsedTime;

					if (yVel < 0) {
						yVel = 0;
					}
				}

				if (yVel < 0) {
					yVel += yAcc*elapsedTime;

					if (yVel > 0) {
						yVel = 0;
					}
				}
			}

			y += yVel*elapsedTime;

			if (y < -12) {
				y = -12;
				yVel = 0;
			}
			if (y > 12) {
				y = 12;
				yVel = 0;
			}

			if (reloading > 0) {
				reloading -= elapsedTime;
				if (reloading < 0) {
					reloading = 0;
				}
			}

			float angle = 35*(yVel/maxYVel);
			drawObject.setRotation({2*3.14159f*(angle/360), 0, 0});
			drawObject.setPos({-12, y, 0});
			
			accDir = 0;
		}

		void accLeft(float elapsedTime) {
			accDir = -1;
		}

		void accRight(float elapsedTime) {
			accDir = 1;
		}

		tuple<Bullet, bool> shot() {
			tuple<Bullet, bool> result;
			get<1>(result) = false;

			if (reloading == 0 && bullets > 0) {
				reloading = reloadTime;
				bullets--;

				get<0>(result) =  Bullet(-12+3, y);
				get<1>(result) = true;
			}

			return result;
		}

		tuple<Laser, bool> shootLaser(float elapsedTime) {
			tuple<Laser, bool> result;
			get<1>(result) = false;

			if (laserTime < laserTimeMax) {
				get<0>(result) = Laser(-12+3, y);
				get<1>(result) = true;

				laserTime += elapsedTime;
			}

			return result;
		}

		void destroydBlock() {
			bullets += 3;
			score++;
		}

		void crash() {
			health--;
		}

		string getHealthString() {
			string healthString = "";

			for (int i = 0; i<health; i++) {
				healthString += "I";
			}

			return healthString;
		}

		string getBulletString() {
			return to_string(bullets);
		}

		string getScoreString() {
			return to_string(score);
		}

		bool end() {
			return health <= 0;
		}
};

class End {
	public:
		Object drawObject;

		End() {
			drawObject = Object::loadFromFile("Objects/end");
		}

		void update() {
			drawObject.setPos({-12+5+1, 0, -1-0.3});
		}
};

class GameTier {
	public:
		int scoreNeeded;
		float blockVelMin;
		float blockVelMax;
		float nextBlockSpawnMin;
		float nextBlockSpawnMax;

		GameTier() {}

		GameTier(int scoreNeeded, float blockVelMin, float blockVelMax, float nextBlockSpawnMin, float nextBlockSpawnMax): scoreNeeded(scoreNeeded), blockVelMin(blockVelMin), blockVelMax(blockVelMax), nextBlockSpawnMin(nextBlockSpawnMin), nextBlockSpawnMax(nextBlockSpawnMax) {}
};



/*
 * Main: Main
 */

class Demo : public olc::PixelGameEngine {
	public:
		StateManager stateManager;

		int lastScore;

		map<string, olc::Sprite*> sprites;

		Demo();

		bool OnUserCreate() override;
		bool OnUserUpdate(float elapsedTime) override;
};

class MenuState : public State {
	public:
		StateManager* stateManager;
		Demo* pgengine;

		olc::Sprite* putinImage;
		bool showPutin;

		MenuState(StateManager* stateManager, Demo* pgengine): stateManager(stateManager), pgengine(pgengine) {
			name = "Menu";

			putinImage = pgengine->sprites["putin"];

			showPutin = true;
		}

		void onStart() {}

		bool onUpdate(float elapsedTime) {
			pgengine->FillRect(0, 0, pgengine->ScreenWidth(), pgengine->ScreenHeight(), olc::Pixel(255, 255, 255));

			if (showPutin) {
				pgengine->DrawSprite(250-putinImage->width/2, 170-putinImage->height/2, putinImage);
			}

			int x1 = 250-29*4;
			int x2 = 250-21*4;
			int x3 = 250-24*4;
			pgengine->DrawString(x1, 320, "Press [ SPACE ] to start game", olc::Pixel(0, 0, 0));
			pgengine->DrawString(x2, 320+20, "Press [ ESC ] to exit", olc::Pixel(0, 0, 0));
			pgengine->DrawString(x3, 320+40, "Press [ h ] to show help", olc::Pixel(0, 0, 0));

			if (pgengine->GetKey(olc::Key::SPACE).bPressed) {
				stateManager->setState("Game");
			}
			if (pgengine->GetKey(olc::Key::H).bPressed) {
				stateManager->setState("Help");
			}
			if (pgengine->GetKey(olc::Key::ESCAPE).bPressed) {
				return false;
			} else {
				return true;
			}
		}

		void onEnd() {}
};

class GameState : public State {
	public:
		StateManager* stateManager;
		Demo* pgengine;

		Engine engine;

		Player player;
		vector<Bullet> bullets;
		vector<Block> blocks;
		vector<Laser> lasers;

		End end;

		float nextBlock;
		float blockSpawnTime;

		int tier;
		vector<GameTier> tiers;

		GameState(StateManager* stateManager, Demo* pgengine): stateManager(stateManager), pgengine(pgengine) {
			name = "Game";

			engine = Engine();
			
			blockSpawnTime = 3;

			tiers.push_back(GameTier( 4, 1.5, 1.9, 1.5, 2.5));
			tiers.push_back(GameTier( 9, 1.7, 2.1, 1.4, 2.3));
			tiers.push_back(GameTier(15, 1.9, 2.3, 1.3, 2.2));
			tiers.push_back(GameTier(21, 2.1, 2.5, 1.2, 2.1));
			tiers.push_back(GameTier(24, 2.3, 2.7, 1.1, 1.9));
		}

		void onStart() {
			player = Player();

			end = End();

			bullets.clear();
			bullets.shrink_to_fit();

			blocks.clear();
			blocks.shrink_to_fit();

			lasers.clear();
			lasers.shrink_to_fit();

			nextBlock = 0;

			tier = 0;
		}

		bool onUpdate(float elapsedTime) {
			pgengine->FillRect(0, 0, pgengine->ScreenWidth(), pgengine->ScreenHeight(), olc::Pixel(255, 255, 255));

			GameTier currentTier = tiers[tier];
			vector<Bullet> newBullets;
			vector<Block> newBlocks;
			vector<Laser> newLasers;

			/* player update */
			player.update(elapsedTime);

			/* bullet update */
			for (vector<Bullet>::iterator bullet = bullets.begin(); bullet != bullets.end(); bullet++) {
				bullet->update(elapsedTime);

				if (!bullet->end()) {
					newBullets.push_back(*bullet);
				}
			}
			bullets = newBullets;

			/* lasers update */
			for (vector<Laser>::iterator laser = lasers.begin(); laser != lasers.end(); laser++) {
				laser->update(elapsedTime);

				if (!laser->end()) {
					newLasers.push_back(*laser);
				}
			}
			lasers = newLasers;

			/* block update */
			if (nextBlock > 0) {
				nextBlock -= elapsedTime;

				if (nextBlock < 0 || blocks.size() == 0) {
					nextBlock = 0;
				}
			}

			if (nextBlock == 0) {
				nextBlock = randFloat(currentTier.nextBlockSpawnMin, currentTier.nextBlockSpawnMax, 3);

				blocks.push_back(Block::spawn(randFloat(currentTier.blockVelMin, currentTier.blockVelMax, 3)));
			}

			for (vector<Block>::iterator block = blocks.begin(); block != blocks.end(); block++) {
				block->update(elapsedTime);

				if (block->end()) {
					player.crash();
				} else {
					newBlocks.push_back(*block);
				}
			}
			blocks = newBlocks;

			/* bullet-block collisions */
			newBullets.clear();
			newBullets.shrink_to_fit();
			for (vector<Bullet>::iterator bullet = bullets.begin(); bullet != bullets.end(); bullet++) {
				bool collided = false;

				newBlocks.clear();
				newBlocks.shrink_to_fit();
				for (vector<Block>::iterator block = blocks.begin(); block != blocks.end(); block++) {
					if (bullet->collide(*block)) {
						collided = true;

						player.destroydBlock();
					} else {
						newBlocks.push_back(*block);
					}
				}
				blocks = newBlocks;

				if (!collided) {
					newBullets.push_back(*bullet);
				}
			}
			bullets = newBullets;

			/* end update */
			end.update();

			/* player dead */
			bool gameEnd = player.end();

			/* tier update */
			if (tier < tiers.size()-1) {
				if (player.score >= currentTier.scoreNeeded) {
					tier++;
					currentTier = tiers[tier];
				}
			}
			
			/* drawing 3d */
			vector3d center = {-30.0, player.y, 20.0};
			vector3d direction = {35.0, 0.0, -20.0};
			vector3d view1 = {0.0, 20.0, 0.0};
			vector3d view2 = {-(4.0*5*2)/sqrt(13), 0.0, -(4.0*5*3)/sqrt(13)};

			engine.setCamera(Camera(center, direction, view1, view2));

			vector<Object> renderObjects;

			renderObjects.push_back(player.drawObject);

			for (vector<Bullet>::iterator bullet = bullets.begin(); bullet != bullets.end(); bullet++) {
				renderObjects.push_back(bullet->drawObject);
			}

			for (vector<Block>::iterator block = blocks.begin(); block != blocks.end(); block++) {
				renderObjects.push_back(block->drawObject);
			}

			for (vector<Laser>::iterator laser = lasers.begin(); laser != lasers.end(); laser++) {
				renderObjects.push_back(laser->drawObject);
			}

			renderObjects.push_back(end.drawObject);

			engine.renderObjects(renderObjects, (*pgengine));

			/* drawing 2d */
			pgengine->DrawString(10, 10+15*0, "Health: "+player.getHealthString(), olc::Pixel(0, 0, 0));
			pgengine->DrawString(10, 10+15*1, "Bullets: "+player.getBulletString(), olc::Pixel(0, 0, 0));
			pgengine->DrawString(10, 10+15*2, "Score: "+player.getScoreString(), olc::Pixel(0, 0, 0));

			/* user input */
			if (pgengine->GetKey(olc::Key::RIGHT).bHeld) {
				player.accRight(elapsedTime);
			}
			if (pgengine->GetKey(olc::Key::LEFT).bHeld) {
				player.accLeft(elapsedTime);
			}
			if (pgengine->GetKey(olc::Key::UP).bHeld) {
				tuple<Bullet, bool> shotResult = player.shot();
				if (get<1>(shotResult)) {
					bullets.push_back(get<0>(shotResult));
				}
			}
			if (pgengine->GetKey(olc::Key::DOWN).bHeld) {
				tuple<Laser, bool> shotResult = player.shootLaser(elapsedTime);
				if (get<1>(shotResult)) {
					lasers.push_back(get<0>(shotResult));
				}
			}

			/* state changing */
			if (gameEnd) {
				stateManager->setState("GameOver");
			}

			return true;
		}

		void onEnd() {
			pgengine->lastScore = player.score;
		}
};

class GameOverState : public State {
	public:
		StateManager* stateManager;
		Demo* pgengine;

		GameOverState(StateManager* stateManager, Demo* pgengine): stateManager(stateManager), pgengine(pgengine) {
			name = "GameOver";
		}

		void onStart() {
			
		}

		bool onUpdate(float elapsedTime) {
			pgengine->FillRect(0, 0, pgengine->ScreenWidth(), pgengine->ScreenHeight(), olc::Pixel(0, 0, 0));

			pgengine->DrawString(40, 40, "Game Over", olc::Pixel(255, 255, 255), 2);
			pgengine->DrawString(40, 75, "Score: "+to_string(pgengine->lastScore), olc::Pixel(255, 255, 255));
			pgengine->DrawString(40, 90, "Press [ SPACE ] to go back to menu", olc::Pixel(255, 255, 255));

			if (pgengine->GetKey(olc::Key::SPACE).bPressed) {
				stateManager->setState("Menu");
			}

			return true;
		}

		void onEnd() {
			
		}
};

class HelpState : public State {
	public:
		StateManager* stateManager;
		Demo* pgengine;

		HelpState(StateManager* stateManager, Demo* pgengine): stateManager(stateManager), pgengine(pgengine) {
			name = "Help";
		}

		void onStart() {}

		bool onUpdate(float elapsedTime) {
			pgengine->FillRect(0, 0, pgengine->ScreenWidth(), pgengine->ScreenHeight(), olc::Pixel(255, 255, 255));

			int x1 = 460-46*8;
			int x2 = 460-21*8;
			int x3 = 460-34*8;
			pgengine->DrawString(x1, 40+15*0, "Use left and right arrow to control space ship", olc::Pixel(0, 0, 0));
			pgengine->DrawString(x2, 40+15*1, "Use up arrow to shoot", olc::Pixel(0, 0, 0));
			pgengine->DrawString(x3, 460-15*1, "Press [ ESC ] to go back to menu", olc::Pixel(0, 0, 0));

			if (pgengine->GetKey(olc::Key::ESCAPE).bPressed) {
				stateManager->setState("Menu");
			}

			return true;
		}

		void onEnd() {}
};



Demo::Demo() {
	sAppName = "ClickerinoCpp";
}

bool Demo::OnUserCreate() {
	olc::Sprite* putinSprite = new olc::Sprite("Sprites/putinASCII.png");
	sprites.insert(make_pair("putin", putinSprite));

	stateManager = StateManager();

	State* gameState = new GameState((&stateManager), this);
	stateManager.addState(gameState);

	State* menuState = new MenuState((&stateManager), this);
	stateManager.addState(menuState);

	State* gameOverState = new GameOverState((&stateManager), this);
	stateManager.addState(gameOverState);

	State* helpState = new HelpState((&stateManager), this);
	stateManager.addState(helpState);

	lastScore = -1;

	stateManager.setState("Menu");

	return true;
}

bool Demo::OnUserUpdate(float elpasedTime) {
	return stateManager.update(elpasedTime);
}



int main() {
	srand(time(NULL));

	Demo demo;
	if (demo.Construct(500, 500, 1, 1)) {
        demo.Start();
    }

	return 0;
}