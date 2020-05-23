/*
Niniejszy program jest wolnym oprogramowaniem; możesz go
rozprowadzać dalej i / lub modyfikować na warunkach Powszechnej
Licencji Publicznej GNU, wydanej przez Fundację Wolnego
Oprogramowania - według wersji 2 tej Licencji lub(według twojego
wyboru) którejś z późniejszych wersji.

Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
ZASTOSOWAŃ.W celu uzyskania bliższych informacji sięgnij do
Powszechnej Licencji Publicznej GNU.

Z pewnością wraz z niniejszym programem otrzymałeś też egzemplarz
Powszechnej Licencji Publicznej GNU(GNU General Public License);
jeśli nie - napisz do Free Software Foundation, Inc., 59 Temple
Place, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE

#include<iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "myCube.h"
#include "myTeapot.h"
#include <map>
#include <fstream>

namespace mathematics {
	void min(float* value, float min) {
		if (*value > min) return;
		else *value = min;
	}

	void min(int* value, int min) {
		if (*value > min) return;
		else *value = min;
	}

	void max(float* value, float max) {
		if (*value < max) return;
		else *value = max;
	}

	void max(int* value, int max) {
		if (*value < max) return;
		else *value = max;
	}
	
	void clamp(float* value, float min, float max) {
		mathematics::min(value, min);
		mathematics::max(value, max);
	}

	void clamp(int* value, int min, int max) {
		mathematics::min(value, min);
		mathematics::max(value, max);
	}
}

namespace printg {
	void vector(std::vector<std::string> v, std::string title) {
		std::cout << title << std::endl;
		for (int i = 0; i < v.size(); i++) std::cout << "<->" << v[i];
		std::cout << std::endl;
	}

	void vector(std::vector<std::string> v) {
		for (int i = 0; i < v.size(); i++) std::cout << "<->" << v[i];
		std::cout << std::endl;
	}

	void last(std::vector<float>* v, int amount) {
		int last = v->size() - amount;
		mathematics::clamp(&last, 0, v->size());
		for (int i = v->size() - 1;  i >= last; i--) std::cout << v->at(i) << " ";
		std::cout << std::endl;
	}
}

namespace objects {
	enum Figure { board };

	std::map<objects::Figure, glm::mat4> M;
	std::map<objects::Figure, int> textureUnitA;
	std::map<objects::Figure, int> textureUnitB;
	std::map<objects::Figure, int> textureUnitNumberA;
	std::map<objects::Figure, int> textureUnitNumberB;
	std::map<objects::Figure, GLuint> textureMap;
	std::map<objects::Figure, GLuint> specularMap;
	std::map<objects::Figure, std::vector<float> > vertices;
	std::map<objects::Figure, std::vector<float> > normals;
	std::map<objects::Figure, std::vector<float> > texCoords;
	std::map<objects::Figure, int> vertexCount;
}

namespace observer {
	glm::vec3 position;
	glm::vec3 lookAtPosition;
	int rotationDirectionX;
	int rotationDirectionY;
	int moveLeftRightDirection;
	int moveForwardDirection;
	float distance;
	const float rotationSpeed = 0.1;
	const float moveSpeed = 0.2;
	float totalRotation;
	float totalYaw;

	void init() {
		observer::position = glm::vec3(0.0f, 0.0f, -3.0f);
		observer::lookAtPosition = glm::vec3(0.0f, 0.0f, 0.0f);
		observer::distance = 3;
		observer::rotationDirectionX = 0;
		observer::rotationDirectionY = 0;
		observer::moveLeftRightDirection = 0;
		observer::moveForwardDirection = 0;
		observer::totalRotation = 0;
		observer::totalYaw = 0;
	}

	void move() {
		glm::vec3 forward = glm::normalize(observer::lookAtPosition - observer::position);
		glm::vec3 perpendicular = glm::normalize(glm::vec3(-forward.z, 0.0f, forward.x));
		// move forward
		observer::position += forward * (float)observer::moveForwardDirection * observer::moveSpeed;

		// move left / right
		observer::position += perpendicular * (float)observer::moveLeftRightDirection * observer::moveSpeed;

		observer::lookAtPosition = observer::position +  forward * observer::distance;
	}

	// positive angles result clockwise rotations
	void rotate() {
		float yaw = observer::rotationDirectionX * observer::rotationSpeed;
		observer::totalYaw += yaw;
		mathematics::clamp(&observer::totalYaw, -1.5f, 1.5f);
		// yaw
		float dy = observer::distance * sin(observer::totalYaw);
		float diameter = observer::distance * cos(observer::totalYaw);

		float rotation = observer::rotationDirectionY * observer::rotationSpeed;
		observer::totalRotation = (observer::totalRotation + rotation);
		// rotation
		float dx = diameter * sin(observer::totalRotation);
		float dz = diameter * cos(observer::totalRotation);


		observer::lookAtPosition = observer::position + glm::vec3(dx, dy, dz);
	}
}


namespace render {
	glm::mat4 V;
	glm::mat4 P;
	ShaderProgram* shaderProgram;
	float aspectRatio = 1;

	GLuint readTexture(const char* filename, int textureUnit) {
		GLuint tex;
		glActiveTexture(textureUnit); //Wczytanie do pamięci komputera
		std::vector<unsigned char> image;   //Alokuj wektor do wczytania obrazka
		unsigned width, height;   //Zmienne do których wczytamy wymiary obrazka
								 //Wczytaj obrazek
		unsigned error = lodepng::decode(image, width, height, filename);//Import do pamięci karty graficznej
		glGenTextures(1, &tex); //Zainicjuj jeden uchwyt
		glBindTexture(GL_TEXTURE_2D, tex); //Uaktywnij uchwyt
										   //Wczytaj obrazek do pamięci KG skojarzonej z uchwytem
		glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		return tex;
	}

	void init() {
		render::P = glm::perspective(50.0f * PI / 180.0f, render::aspectRatio, 0.01f, 50.0f);
		render::shaderProgram = new ShaderProgram("v_simplest.glsl", NULL, "f_simplest.glsl");
	}



	void render(objects::Figure figure) {

		int textureUnitA = objects::textureUnitA[figure];
		int textureUnitB = objects::textureUnitB[figure];
		int textureUnitNumberA = objects::textureUnitNumberA[figure];
		int textureUnitNumberB = objects::textureUnitNumberB[figure];
		float* vertices = objects::vertices[figure].data();
		float* normals = objects::normals[figure].data();
		float* texCoords = objects::texCoords[figure].data();
		GLuint textureMap = objects::textureMap[figure];
		GLuint specularMap = objects::specularMap[figure];
		int vertexCount = objects::vertexCount[figure];
		//for (int i = 0; i < 36; i++) std::cout << vertices[i] << " ";

		glm::mat4 M = objects::M[figure];

		render::V = glm::lookAt(
			observer::position,
			observer::lookAtPosition,
			glm::vec3(0.0f, 1.0f, 0.0f));
		

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		render::shaderProgram->use();//Aktywacja programu cieniującego
		//Przeslij parametry programu cieniującego do karty graficznej
		glUniformMatrix4fv(render::shaderProgram->u("P"), 1, false, glm::value_ptr(render::P));
		glUniformMatrix4fv(render::shaderProgram->u("V"), 1, false, glm::value_ptr(render::V));
		glUniformMatrix4fv(render::shaderProgram->u("M"), 1, false, glm::value_ptr(M));
		glUniform1i(render::shaderProgram->u("textureMap"), textureUnitNumberA);
		glUniform1i(render::shaderProgram->u("specularMap"), textureUnitNumberB);
		glUniform4f(render::shaderProgram->u("lightPosition"), 0, 3, -6, 1);

		glEnableVertexAttribArray(render::shaderProgram->a("vertexPosition"));  //Włącz przesyłanie danych do atrybutu vertex
		glVertexAttribPointer(render::shaderProgram->a("vertexPosition"), 4, GL_FLOAT, false, 0, vertices); //Wskaż tablicę z danymi dla atrybutu vertex

		glEnableVertexAttribArray(render::shaderProgram->a("vertexNormal"));  //Włącz przesyłanie danych do atrybutu normal
		glVertexAttribPointer(render::shaderProgram->a("vertexNormal"), 4, GL_FLOAT, false, 0, normals); //Wskaż tablicę z danymi dla atrybutu normal

		glEnableVertexAttribArray(render::shaderProgram->a("textureCoord"));
		glVertexAttribPointer(render::shaderProgram->a("textureCoord"), 2, GL_FLOAT, false, 0, texCoords);

		glActiveTexture(textureUnitA);
		glBindTexture(GL_TEXTURE_2D, textureMap);
		glActiveTexture(textureUnitB);
		glBindTexture(GL_TEXTURE_2D, specularMap);

		glDrawArrays(GL_TRIANGLES, 0, vertexCount); //Narysuj obiekt

		glDisableVertexAttribArray(render::shaderProgram->a("vertexPosition"));  //Wyłącz przesyłanie danych do atrybutu vertex
		glDisableVertexAttribArray(render::shaderProgram->a("vertexNormal"));  //Wyłącz przesyłanie danych do atrybutu normal
		glDisableVertexAttribArray(render::shaderProgram->a("textureCoord"));
	}

	void setAspectRatio(float aspectRatio) {
		render::aspectRatio = aspectRatio;
	}

	void unload() {
		delete render::shaderProgram;
	}
}

namespace transformations {
	enum Axis { x, y, z };

	void move(objects::Figure figure, transformations::Axis axis, float distance) {
		switch(axis) {
			case transformations::Axis::x: {
				objects::M[figure] = glm::translate(objects::M[figure], glm::vec3(distance, 0.0f, 0.0f));
				break;
			}
			case transformations::Axis::y: {
				objects::M[figure] = glm::translate(objects::M[figure], glm::vec3(0.0f, distance, 0.0f));
				break;
			}
			case transformations::Axis::z: {
				objects::M[figure] = glm::translate(objects::M[figure], glm::vec3(0.0f, 0.0f, distance));
				break;
			}
		}
	}

	void rotate(objects::Figure figure, transformations::Axis axis, float angle) {
		switch (axis) {
			case transformations::Axis::x: {
				objects::M[figure] = glm::rotate(objects::M[figure], angle, glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			}
			case transformations::Axis::y: {
				objects::M[figure] = glm::rotate(objects::M[figure], angle, glm::vec3(0.0f, 1.0f, 0.0f));
				break;
			}
			case transformations::Axis::z: {
				objects::M[figure] = glm::rotate(objects::M[figure], angle, glm::vec3(0.0f, 0.0f, 1.0f));
				break;
			}
		}
	}

	void scale(objects::Figure figure, transformations::Axis axis, float scale) {
		switch (axis) {
			case transformations::Axis::x: {
				objects::M[figure] = glm::scale(objects::M[figure], glm::vec3(scale, 1.0f, 1.0f));
				break;

			}
			case transformations::Axis::y: {
				objects::M[figure] = glm::scale(objects::M[figure], glm::vec3(1.0f, scale, 1.0f));
				break;
			}
			case transformations::Axis::z: {
				objects::M[figure] = glm::scale(objects::M[figure], glm::vec3(1.0f, 1.0f, scale));
				break;
			}
		}
	}
}

namespace transformationsInject {
	void move(glm::mat4* M, transformations::Axis axis, float distance) {
		switch (axis) {
			case transformations::Axis::x: {
				*M = glm::translate(*M, glm::vec3(distance, 0.0f, 0.0f));
				break;
			}
			case transformations::Axis::y: {
				*M = glm::translate(*M, glm::vec3(0.0f, distance, 0.0f));
				break;
			}
			case transformations::Axis::z: {
				*M = glm::translate(*M, glm::vec3(0.0f, 0.0f, distance));
				break;
			}
		}
	}

	void rotate(glm::mat4* M, transformations::Axis axis, float angle) {
		switch (axis) {
			case transformations::Axis::x: {
				*M = glm::rotate(*M, angle, glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			}
			case transformations::Axis::y: {
				*M = glm::rotate(*M, angle, glm::vec3(0.0f, 1.0f, 0.0f));
				break;
			}
			case transformations::Axis::z: {
				*M = glm::rotate(*M, angle, glm::vec3(0.0f, 0.0f, 1.0f));
				break;
			}
		}
	}

	void scale(glm::mat4* M, transformations::Axis axis, float scale) {
		switch (axis) {
			case transformations::Axis::x: {
				*M = glm::scale(*M, glm::vec3(scale, 1.0f, 1.0f));
				break;
			}
			case transformations::Axis::y: {
				*M  = glm::scale(*M, glm::vec3(1.0f, scale, 1.0f));
				break;
			}
			case transformations::Axis::z: {
				*M = glm::scale(*M, glm::vec3(1.0f, 1.0f, scale));
				break;
			}
		}
	}

}

namespace model {
	struct vertex {
		float x;
		float y;
		float z;
	};

	struct face {
		int v1;
		int n1;
		int t1;

		int v2;
		int n2;
		int t2;

		int v3;
		int n3;
		int t3;

		int v4;
		int n4;
		int t4;
	};

	struct normal {
		float x;
		float y;
		float z;
	};

	struct texture {
		float x;
		float y;
	};

	namespace print {
		void vertex(model::vertex v) {
			std::cout << "x: " << v.x << " y: " << v.y << " z: " << v.z << std::endl;
		}

		void normal(model::normal n) {
			std::cout << "x: " << n.x << " y: " << n.y << " z: " << n.z << std::endl;
		}

		void texture(model::texture t) {
			std::cout << "x: " << t.x << " y: " << t.y << std::endl;
		}

		void face(model::face f) {
			std::cout << "Vertex 1: "
				<< "\n\tvertex_no: " << f.v1
				<< "\n\tnormal_no: " << f.n1
				<< "\n\ttexture_no: " << f.t1 << std::endl;

			std::cout << "Vertex 2: "
				<< "\n\tvertex_no: " << f.v2
				<< "\n\tnormal_no: " << f.n2
				<< "\n\ttexture_no: " << f.t2 << std::endl;

			std::cout << "Vertex 3: "
				<< "\n\tvertex_no: " << f.v3
				<< "\n\tnormal_no: " << f.n3
				<< "\n\ttexture_no: " << f.t3 << std::endl;

			std::cout << "Vertex 4: "
				<< "\n\tvertex_no: " << f.v4
				<< "\n\tnormal_no: " << f.n4
				<< "\n\ttexture_no: " << f.t4 << std::endl;
		}
	}

	std::vector<std::string> split(std::string line, std::string delimiter) {
		std::vector<std::string> tokens;
		int start = 0;
		int end = line.find(delimiter);
		int delimiterLength = delimiter.length();
		while (end != std::string::npos) {
			tokens.push_back(line.substr(start, end - start));
			start = end + delimiterLength;
			end = line.find(delimiter, start);
		}
		tokens.push_back(line.substr(start, std::string::npos));
		return tokens;
	}

	void flatten(std::vector<float>* dest, model::vertex vertex) {
		dest->push_back(vertex.x);
		dest->push_back(vertex.y);
		dest->push_back(vertex.z);
		dest->push_back(1.0f);
	}

	void flatten(std::vector<float>* dest, model::normal normal) {
		dest->push_back(normal.x);
		dest->push_back(normal.y);
		dest->push_back(normal.z);
		dest->push_back(0.0f);
	}

	void flatten(std::vector<float>* dest, model::texture texture) {
		dest->push_back(texture.x);
		dest->push_back(texture.y);
	}

	void inflateToStructs(std::vector<model::vertex> * vertexes, std::vector<model::normal> * normals, std::vector<model::face> * faces, std::vector<model::texture>* textures, std::string filename) {
		std::cout << "Inflating to vectors of structs" << std::endl;
		int line_no = 0;
		std::ifstream file(filename);
		while (!file.eof()) {
			std::string line;
			std::getline(file, line);
			std::vector<std::string> tokens = model::split(line, " ");

			std::cout << "Reading line: " << line_no << std::endl;
			std::cout << "Content: "; printg::vector(tokens);

			std::string type = tokens[0];
			if (type == "v") {
				std::cout << "Line type: V" << std::endl;
				model::vertex v;
				v.x = std::stof(tokens[1]);
				v.y = std::stof(tokens[2]);
				v.z = std::stof(tokens[3]);
				model::print::vertex(v);
				vertexes->push_back(v);
			}
			else if (type == "vn") {
				model::normal n;
				n.x = std::stof(tokens[1]);
				n.y = std::stof(tokens[2]);
				n.z = std::stof(tokens[3]);
				model::print::normal(n);
				normals->push_back(n);
			}
			else if (type == "vt") {
				std::cout << "Line type: VT" << std::endl;
				model::texture t;
				t.x = std::stof(tokens[1]);
				t.y = std::stof(tokens[2]);
				model::print::texture(t);
				textures->push_back(t);
			}
			else if (type == "f") {
				model::face face;
				std::vector<std::string> subtokens1 = model::split(tokens[1], "/");
				face.v1 = std::stoi(subtokens1[0]) - 1;
				face.t1 = (subtokens1[1] == "")? -1.0f : std::stoi(subtokens1[1]) - 1;
				face.n1 = std::stoi(subtokens1[2]) - 1;

				std::vector<std::string> subtokens2 = model::split(tokens[2], "/");
				face.v2 = std::stoi(subtokens2[0]) - 1;
				face.t2 = (subtokens2[1] == "") ? -1 : std::stoi(subtokens2[1]) - 1;
				face.n2 = std::stoi(subtokens2[2]) - 1;

				std::vector<std::string> subtokens3 = model::split(tokens[3], "/");
				face.v3 = std::stoi(subtokens3[0]) - 1;
				face.t3 = (subtokens3[1] == "") ? -1 : std::stoi(subtokens3[1]) - 1;
				face.n3 = std::stoi(subtokens3[2]) - 1;

				std::vector<std::string> subtokens4 = model::split(tokens[4], "/");
				face.v4 = std::stoi(subtokens4[0]) - 1;
				face.t4 = (subtokens4[1] == "") ? -1 : std::stoi(subtokens4[1]) - 1;
				face.n4 = std::stoi(subtokens4[2]) - 1;

				model::print::face(face);
				faces->push_back(face);
			}
			line_no++;
		}
	}

	void flattenToArrays(std::vector<model::vertex>* vertexes, std::vector<model::normal>* normals, std::vector<model::face>* faces, std::vector<model::texture>* textures, std::vector<float>* vertexesFlatten, std::vector<float>* normalsFlatten, std::vector<float>* texturesFlatten) {
		std::cout << "Flattening to arrays" << std::endl;
		std::cout << faces->size() << std::endl;
		for (unsigned int i = 0; i < faces->size(); i++) {
			/*
				1     2
				*-----*
				|  /  |
				*-----*
				4     3
			*/

			std::cout << "face_no: " << i << std::endl;
			model::face face = faces->at(i);
			model::print::face(face);
			// upper triangle
			// vertices
			model::flatten(vertexesFlatten, vertexes->at(face.v4));
			std::cout << "Tailing vertexesFlatten: ";
			printg::last(vertexesFlatten, 4);
			model::flatten(vertexesFlatten, vertexes->at(face.v2));
			std::cout << "Tailing vertexesFlatten: ";
			printg::last(vertexesFlatten, 4);
			model::flatten(vertexesFlatten, vertexes->at(face.v1));
			std::cout << "Tailing vertexesFlatten: ";
			printg::last(vertexesFlatten, 4);

			// textures
			if (face.t1 != -1 && face.t2 != -1 && face.t3 != -1) {
				model::flatten(texturesFlatten, textures->at(face.t4));
				std::cout << "Tailing texturesFlatten: ";
				printg::last(texturesFlatten, 2);
				model::flatten(texturesFlatten, textures->at(face.t2));
				std::cout << "Tailing texturesFlatten: ";
				printg::last(texturesFlatten, 2);
				model::flatten(texturesFlatten, textures->at(face.t1));
				std::cout << "Tailing texturesFlatten: ";
				printg::last(texturesFlatten, 2);
			}
			else {
				for (int j = 0; j < 6; j++) texturesFlatten->push_back(0);
			}

			// normals
			model::flatten(normalsFlatten, normals->at(face.n4));
			std::cout << "Tailing normalsFlatten: ";
			printg::last(normalsFlatten, 4);
			model::flatten(normalsFlatten, normals->at(face.n2));
			std::cout << "Tailing normalsFlatten: ";
			printg::last(normalsFlatten, 4);
			model::flatten(normalsFlatten, normals->at(face.n1));
			std::cout << "Tailing normalsFlatten: ";
			printg::last(normalsFlatten, 4);

			// bottom triangle
			// vertices
			model::flatten(vertexesFlatten, vertexes->at(face.v4));
			std::cout << "Tailing vertexesFlatten: ";
			printg::last(vertexesFlatten, 4);
			model::flatten(vertexesFlatten, vertexes->at(face.v3));
			std::cout << "Tailing vertexesFlatten: ";
			printg::last(vertexesFlatten, 4);
			model::flatten(vertexesFlatten, vertexes->at(face.v2));
			std::cout << "Tailing vertexesFlatten: ";
			printg::last(vertexesFlatten, 4);

			//textures
			if (face.t2 != -1 && face.t3 != -1 && face.t4 != -1) {
				model::flatten(texturesFlatten, textures->at(face.t4));
				std::cout << "Tailing texturesFlatten: ";
				printg::last(texturesFlatten, 2);
				model::flatten(texturesFlatten, textures->at(face.t3));
				std::cout << "Tailing texturesFlatten: ";
				printg::last(texturesFlatten, 2);
				model::flatten(texturesFlatten, textures->at(face.t2));
				std::cout << "Tailing texturesFlatten: ";
				printg::last(texturesFlatten, 2);
			}
			else {
				for (int j = 0; j < 6; j++) texturesFlatten->push_back(0);
			}

			// normals
			model::flatten(normalsFlatten, normals->at(face.n4));
			std::cout << "Tailing normalsFlatten: ";
			printg::last(normalsFlatten, 4);
			model::flatten(normalsFlatten, normals->at(face.n3));
			std::cout << "Tailing normalsFlatten: ";
			printg::last(normalsFlatten, 4);
			model::flatten(normalsFlatten, normals->at(face.n2));
			std::cout << "Tailing normalsFlatten: ";
			printg::last(normalsFlatten, 4);
		}

	}

	void read(std::string filename, objects::Figure figure) {

		std::cout << "Reading model: " << filename << std::endl;
		std::vector<model::vertex> vertexes;
		std::vector<model::normal> normals;
		std::vector<model::face> faces;
		std::vector<model::texture> textures;

		std::vector<float> vertexesResult;
		std::vector<float> normalsResult;
		std::vector<float> texturesResult;
		
		model::inflateToStructs(&vertexes, &normals, &faces, &textures, filename);
		model::flattenToArrays(&vertexes, &normals, &faces, &textures, &vertexesResult, &normalsResult, &texturesResult);

		objects::vertexCount[figure] = faces.size() * 6;
		objects::vertices[figure] = vertexesResult;
		objects::normals[figure] = normalsResult;
		objects::texCoords[figure] = texturesResult;
		for (int i = 0; i < 36; i++) std::cout << objects::vertices[figure][i] << " ";
	}
}

namespace pUpdater {
	std::ifstream file;
	objects::Figure figure;
	glm::mat4 M;

	const char* selectFileName() {
		switch (pUpdater::figure) {
			case objects::Figure::board: {
				return "pBoard.properties";
			}
		}
	}

	void updateScale(float x, float y, float z) {
		transformationsInject::scale(&pUpdater::M, transformations::Axis::x, x);
		transformationsInject::scale(&pUpdater::M, transformations::Axis::y, y);
		transformationsInject::scale(&pUpdater::M, transformations::Axis::z, z);
	}

	void updatePosition(float x, float y, float z) {
		transformationsInject::move(&pUpdater::M, transformations::Axis::x, x);
		transformationsInject::move(&pUpdater::M, transformations::Axis::y, y);
		transformationsInject::move(&pUpdater::M, transformations::Axis::z, z);
	}

	void updateRotation(float x, float y, float z) {
		transformationsInject::rotate(&pUpdater::M, transformations::Axis::x, x);
		transformationsInject::rotate(&pUpdater::M, transformations::Axis::y, y);
		transformationsInject::rotate(&pUpdater::M, transformations::Axis::z, z);
	}

	void createAndOpen() {
		std::fstream f(pUpdater::selectFileName(), std::ios::out | std::ios::trunc);
		f << "position: 0 0 0" << std::endl;
		f << "scale: 1 1 1" << std::endl;
		f << "rotation: 0 0 0" << std::endl;
		f.close();

		pUpdater::file.open(pUpdater::selectFileName());
	}

	void update(objects::Figure figure) {
		pUpdater::file.open(pUpdater::selectFileName());
		pUpdater::figure = figure;
		pUpdater::M = glm::mat4(1.0f);

		if (!pUpdater::file) pUpdater::createAndOpen();

		while (!pUpdater::file.eof()) {
			std::string line;
			std::getline(pUpdater::file, line);
			std::vector<std::string> tokens = model::split(line, " ");
			std::string selector = tokens[0];
			printg::vector(tokens);
			if (selector == "position:") pUpdater::updatePosition(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
			else if (selector == "rotation:") pUpdater::updateRotation(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
			else if (selector == "scale:") pUpdater::updateScale(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
		}
		objects::M[figure] = pUpdater::M;
		pUpdater::file.close();
	}
}

namespace board {
	const objects::Figure figure = objects::Figure::board;

	void init() {
		objects::M[board::figure] = glm::mat4(1.0f);
		objects::textureUnitA[board::figure] = GL_TEXTURE0;
		objects::textureUnitB[board::figure] = GL_TEXTURE1;
		objects::textureUnitNumberA[board::figure] = 0;
		objects::textureUnitNumberB[board::figure] = 1;
		objects::textureMap[board::figure] = render::readTexture("metal.png", GL_TEXTURE0);
		objects::specularMap[board::figure] = render::readTexture("metal_spec.png", GL_TEXTURE1);
		model::read("board.obj", board::figure);

		for (int i = 0; i < 36; i++) std::cout << objects::vertices[figure][i] << " ";
	}

	void move(float distance, transformations::Axis axis) {
		transformations::move(board::figure, axis, distance);
	}

	void rotate(float angle, transformations::Axis axis) {
		transformations::rotate(board::figure, axis, angle);
	}

	void scale(float scale, transformations::Axis axis) {
		transformations::scale(board::figure, axis, scale);
	}

	void render() {
		pUpdater::update(board::figure);
		render::render(board::figure);
		// std::cout << figure;
	}

}

namespace debug {
	void printDelimiter() {
		std::cout << "----------------------" << std::endl;
	}

	void printObserverPosition() {
		std::cout << "Observer position: " << std::endl;
		std::cout << "x: " << observer::position.x << " y: " << observer::position.y << " z: " << observer::position.z << std::endl;
	}

	void printObserverLookAtPosition() {
		std::cout << "Look at position: " << std::endl;
		std::cout << "x: " << observer::lookAtPosition.x << " y: " << observer::lookAtPosition.y << " z: " << observer::lookAtPosition.z << std::endl;
	}

	void printObserver() {
		debug::printObserverPosition();
		debug::printObserverLookAtPosition();
	}

	void printTotalRotation() {
		std::cout << "Total rotation: ";
		std::cout << observer::totalRotation << std::endl;
	}

	void printTotalYaw() {
		std::cout << "Total yaw: ";
		std::cout << observer::totalRotation << std::endl;
	}

	void printFooter() {
		std::cout << std::endl << std::endl;
	}

	void printDistance() {
		std::cout << "Distance: ";
		std::cout << observer::distance << std::endl;
	}

	void printAll() {
		debug::printDelimiter();
		debug::printObserver();
		debug::printTotalRotation();
		debug::printTotalYaw();
		debug::printDistance();
		debug::printFooter();
	}
}

float aspectRatio=1;

//Procedura obsługi błędów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void keyCallback(GLFWwindow* window,int key,int scancode,int action,int mods) {
    if (action==GLFW_PRESS) {
        if (key==GLFW_KEY_LEFT) observer::rotationDirectionY = 1;
        if (key==GLFW_KEY_RIGHT) observer::rotationDirectionY = -1;
        if (key==GLFW_KEY_UP) observer::rotationDirectionX = 1;
        if (key==GLFW_KEY_DOWN) observer::rotationDirectionX = -1;
		if (key == GLFW_KEY_W) observer::moveForwardDirection = 1;
		if (key == GLFW_KEY_S) observer::moveForwardDirection = -1;
		if (key == GLFW_KEY_D) observer::moveLeftRightDirection = 1;
		if (key == GLFW_KEY_A) observer::moveLeftRightDirection = -1;
    }
    if (action==GLFW_RELEASE) {
        if (key==GLFW_KEY_LEFT) observer::rotationDirectionY = 0;
        if (key==GLFW_KEY_RIGHT) observer::rotationDirectionY = 0;
        if (key==GLFW_KEY_UP) observer::rotationDirectionX = 0;
        if (key==GLFW_KEY_DOWN) observer::rotationDirectionX = 0;
		if (key == GLFW_KEY_W) observer::moveForwardDirection = 0;
		if (key == GLFW_KEY_S) observer::moveForwardDirection = 0;
		if (key == GLFW_KEY_D) observer::moveLeftRightDirection = 0;
		if (key == GLFW_KEY_A) observer::moveLeftRightDirection = 0;
		if (key == GLFW_KEY_R) observer::init();
    }
}

void windowResizeCallback(GLFWwindow* window,int width,int height) {
    if (height==0) return;
    aspectRatio=(float)width/(float)height;
	render::setAspectRatio(aspectRatio);
    glViewport(0,0,width,height);
}



//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
	//************Tutaj umieszczaj kod, który należy wykonać raz, na początku programu************
	glClearColor(0,0,0,1);
	glEnable(GL_DEPTH_TEST);
	glfwSetWindowSizeCallback(window,windowResizeCallback);
	glfwSetKeyCallback(window,keyCallback);
	observer::init();
	board::init();
	render::init();
	
}


//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
    //************Tutaj umieszczaj kod, który należy wykonać po zakończeniu pętli głównej************
	render::unload();
}

//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window) {
	//************Tutaj umieszczaj kod rysujący obraz******************
	//observer::move();
	observer::rotate();
	observer::move();
	board::render();
	debug::printAll();
    glfwSwapBuffers(window); //Przerzuć tylny bufor na przedni
}


int main(void)
{
	GLFWwindow* window; //Wskaźnik na obiekt reprezentujący okno

	glfwSetErrorCallback(error_callback);//Zarejestruj procedurę obsługi błędów

	if (!glfwInit()) { //Zainicjuj bibliotekę GLFW
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(500, 500, "OpenGL", NULL, NULL);  //Utwórz okno 500x500 o tytule "OpenGL" i kontekst OpenGL.

	if (!window) //Jeżeli okna nie udało się utworzyć, to zamknij program
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Od tego momentu kontekst okna staje się aktywny i polecenia OpenGL będą dotyczyć właśnie jego.
	glfwSwapInterval(1); //Czekaj na 1 powrót plamki przed pokazaniem ukrytego bufora

	if (glewInit() != GLEW_OK) { //Zainicjuj bibliotekę GLEW
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Operacje inicjujące

	//Główna pętla
	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamknięte
	{
        glfwSetTime(0); //Zeruj timer
		drawScene(window); //Wykonaj procedurę rysującą
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Usuń kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajęte przez GLFW
	exit(EXIT_SUCCESS);
}

