/* 
 *	Powershell command to run this:
 * 		g++ Clickerino.cpp -o a.exe -std=c++11 -lgdiplus -lgdi32 -lopengl32 -lglu32; .\a.exe
 * 
 *	ToDo:
 *		convert pairs to tuples
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

using namespace std;



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

		Object() {};

		Object(vector<vector3d> points, vector<tuple<tuple<int, int, int>, Material>> triangles): points(points), triangles(triangles) {
			pos = {0, 0, 0};
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

		void move(vector3d offset) {
			pos.x += offset.x;
			pos.y += offset.y;
			pos.z += offset.z;
		}

		void setPos(vector3d newPos) {
			pos = newPos;
		}

		vector<vector3d> getPoints() {
			vector<vector3d> pointsx;

			for (vector<vector3d>::iterator point = points.begin(); point != points.end(); point++) {
				pointsx.push_back({pos.x+point->x, pos.y+point->y, pos.z+point->z});
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



class Demo : public olc::PixelGameEngine {
	public:
		Engine engine;
		Object testingObject;

		bool running;

		float y;

		Demo() {
			sAppName = "Example";

			engine = Engine();

			Object cube = Object::loadFromFile("Objects/cube");
			Object spaceShip = Object::loadFromFile("Objects/spaceShip2");

			testingObject = Object::copy(spaceShip);
			
			y = 12;
		}

		bool OnUserCreate() override {
			running = false;

			return true;
		}

		bool OnUserUpdate(float fElapsedTime) override {
			FillRect(0, 0, ScreenWidth(), ScreenHeight(), olc::Pixel(255, 255, 255));

			vector3d center = {-20.0, 0.0, 30.0};
			vector3d direction = {20.0, 0.0, -30.0};
			vector3d view1 = {0.0, 20, 0.0};
			vector3d view2 = {-10*sqrt(2), 0.0, -10*sqrt(2)};

			engine.setCamera(Camera(center, direction, view1, view2));

			testingObject.setPos({-12.0, y, 0.0});

			vector<Object> testingObjects;
			testingObjects.push_back(testingObject);

			engine.renderObjects(testingObjects, *this);

			if (GetKey(olc::Key::RIGHT).bHeld) {
				y += 5.*fElapsedTime;
			}
			if (GetKey(olc::Key::LEFT).bHeld) {
				y += -5.*fElapsedTime;
			}

			return true;
		}
};




int main() {
	Demo demo;
	if (demo.Construct(400, 400, 1, 1)) {
        demo.Start();
    }

	return 0;
}