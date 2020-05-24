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
#include <map>
#include <fstream>
#include "models/board/object.obj.h"
#include "models/bishop_w/object.obj.h"


const bool isDebugActive = false;
const bool makeAutoUpdate = true;

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

namespace global {
	glm::vec3 light1Pos;
	glm::vec3 light2Pos;
	const char* settingsPath = "settings.prop";

	void init() {
		light1Pos = glm::vec3(0.0f, 12.0f, -4.0f);
		light2Pos = glm::vec3(0.0f, 12.0f, 4.0f);
	}
}

namespace printg {
	void vector(std::vector<std::string> v, std::string title) {
		if (isDebugActive) {
			std::cout << title << std::endl;
			for (int i = 0; i < v.size(); i++) std::cout << "<->" << v[i];
			std::cout << std::endl;
		}
		
	}

	void vector(std::vector<std::string> v) {
		if (isDebugActive) {
			for (int i = 0; i < v.size(); i++) std::cout << "<->" << v[i];
			std::cout << std::endl;
		}
	}

	void last(std::vector<float>* v, int amount) {
		if (isDebugActive) {
			int last = v->size() - amount;
			mathematics::clamp(&last, 0, v->size());
			for (int i = v->size() - 1; i >= last; i--) std::cout << v->at(i) << " ";
			std::cout << std::endl;
		}
	}
}

namespace objects {
	enum Figure { board,
		bishop_w1, bishop_w2, bishop_b1, bishop_b2,
		king_w, queen_w, king_b, queen_b,
		rook_w1, rook_w2, rook_b1, rook_b2,
		knight_w1, knight_w2, knight_b1, knight_b2,
		pawn_w1, pawn_w2, pawn_w3, pawn_w4, pawn_w5, pawn_w6, pawn_w7, pawn_w8,
		pawn_b1, pawn_b2, pawn_b3, pawn_b4, pawn_b5, pawn_b6, pawn_b7, pawn_b8
	};

	std::map<objects::Figure, glm::mat4> M;
	std::map<objects::Figure, int> textureUnitA;
	std::map<objects::Figure, int> textureUnitB;
	std::map<objects::Figure, int> textureUnitNumberA;
	std::map<objects::Figure, int> textureUnitNumberB;
	std::map<objects::Figure, GLuint> textureMap;
	std::map<objects::Figure, GLuint> specularMap;
	std::map<objects::Figure, float*> vertices;
	std::map<objects::Figure, float*> normals;
	std::map<objects::Figure, float*> texCoords;
	std::map<objects::Figure, int> vertexCount;
	std::map<objects::Figure, const char*> imagePathA;
	std::map<objects::Figure, const char*> imagePathB;
	std::map<objects::Figure, std::string> controllFilePath;
	std::map<objects::Figure, glm::vec3> ambientOcculusion;
	std::map<objects::Figure, float> metallic;
	std::map<objects::Figure, float> roughness;
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
		//glActiveTexture(textureUnit); //Wczytanie do pamięci komputera
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

	GLuint readTextureA(objects::Figure figure) {
		return render::readTexture(objects::imagePathA[figure], objects::textureUnitA[figure]);
	}

	GLuint readTextureB(objects::Figure figure) {
		return render::readTexture(objects::imagePathB[figure], objects::textureUnitB[figure]);
	}

	void init() {
		render::P = glm::perspective(50.0f * PI / 180.0f, render::aspectRatio, 0.01f, 150.0f);
		render::shaderProgram = new ShaderProgram("shaders/v_simplest.glsl", NULL, "shaders/f_simplest.glsl");
	}

	void start() {
		render::V = glm::lookAt(
			observer::position,
			observer::lookAtPosition,
			glm::vec3(0.0f, 1.0f, 0.0f));
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void render(objects::Figure figure) {

		int textureUnitA = objects::textureUnitA[figure];
		int textureUnitB = objects::textureUnitB[figure];
		int textureUnitNumberA = objects::textureUnitNumberA[figure];
		int textureUnitNumberB = objects::textureUnitNumberB[figure];
		float* vertices = objects::vertices[figure];
		float* normals = objects::normals[figure];
		float* texCoords = objects::texCoords[figure];
		GLuint textureMap = objects::textureMap[figure];
		GLuint specularMap = objects::specularMap[figure];
		int vertexCount = objects::vertexCount[figure];
		float roughness = objects::roughness[figure];
		float metallic = objects::metallic[figure];
		glm::vec3 ambientOcculusion = objects::ambientOcculusion[figure];
		//for (int i = 0; i < 36; i++) std::cout << vertices[i] << " ";

		glm::vec3 lightPossitions[] = {
			global::light1Pos,
			global::light2Pos
		};

		glm::mat4 M = objects::M[figure];
		render::shaderProgram->use();//Aktywacja programu cieniującego
		//Przeslij parametry programu cieniującego do karty graficznej
		glUniformMatrix4fv(render::shaderProgram->u("P"), 1, false, glm::value_ptr(render::P));
		glUniformMatrix4fv(render::shaderProgram->u("V"), 1, false, glm::value_ptr(render::V));
		glUniformMatrix4fv(render::shaderProgram->u("M"), 1, false, glm::value_ptr(M));
		glUniform1i(render::shaderProgram->u("textureMap"), textureUnitNumberA);
		glUniform1i(render::shaderProgram->u("specularMap"), textureUnitNumberB);
		glUniform3fv(render::shaderProgram->u("lightPosition"), 2, glm::value_ptr(lightPossitions[0]));
		glUniform4f(render::shaderProgram->u("cameraPosition"), observer::position.x, observer::position.y, observer::position.z, 1);
		glUniform1f(render::shaderProgram->u("metallic"), metallic);
		glUniform1f(render::shaderProgram->u("roughness"), roughness);
		glUniform3f(render::shaderProgram->u("ao"), ambientOcculusion.x, ambientOcculusion.y, ambientOcculusion.z);

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

namespace converter {
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
		void vertex(converter::vertex v) {
			if (isDebugActive) {
				std::cout << "x: " << v.x << " y: " << v.y << " z: " << v.z << std::endl;
			}
		}

		void normal(converter::normal n) {
			if (isDebugActive) {
				std::cout << "x: " << n.x << " y: " << n.y << " z: " << n.z << std::endl;
			}
		}

		void texture(converter::texture t) {
			if (isDebugActive) {
				std::cout << "x: " << t.x << " y: " << t.y << std::endl;
			}
		}

		void face(converter::face f) {
			if (isDebugActive) {
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

	void flatten(std::vector<float>* dest, converter::vertex vertex) {
		dest->push_back(vertex.x);
		dest->push_back(vertex.y);
		dest->push_back(vertex.z);
		dest->push_back(1.0f);
	}

	void flatten(std::vector<float>* dest, converter::normal normal) {
		dest->push_back(normal.x);
		dest->push_back(normal.y);
		dest->push_back(normal.z);
		dest->push_back(0.0f);
	}

	void flatten(std::vector<float>* dest, converter::texture texture) {
		dest->push_back(texture.x);
		dest->push_back(texture.y);
	}

	void removeExtraSpaces(std::string* line) {
		char last = 'a';
		std::string result;
		for (int i = 0; i < line->size(); i++) {
			char c = line->at(i);
			if (c != ' ') {
				result.push_back(c);
			}
			else if (c == ' ' && last != ' ') {
				result.push_back(c);
			}
			last = c;
		}
		*line = result;
	}

	void inflateToStructs(std::vector<converter::vertex> * vertexes, std::vector<converter::normal> * normals, std::vector<converter::face> * faces, std::vector<converter::texture>* textures, std::string filename) {
		std::cout << "Inflating to vectors of structs" << std::endl;
		int line_no = 0;
		std::ifstream file(filename);
		while (!file.eof()) {
			std::string line;
			std::getline(file, line);
			converter::removeExtraSpaces(&line);
			std::vector<std::string> tokens = converter::split(line, " ");

			std::cout << "Reading line: " << line_no << std::endl;
			if (isDebugActive) std::cout << "Content: "; printg::vector(tokens);

			std::string type = tokens[0];
			if (type == "v") {
				if (isDebugActive) std::cout << "Line type: V" << std::endl;
				converter::vertex v;
				v.x = std::stof(tokens[1]);
				v.y = std::stof(tokens[2]);
				v.z = std::stof(tokens[3]);
				converter::print::vertex(v);
				vertexes->push_back(v);
			}
			else if (type == "vn") {
				converter::normal n;
				n.x = std::stof(tokens[1]);
				n.y = std::stof(tokens[2]);
				n.z = std::stof(tokens[3]);
				converter::print::normal(n);
				normals->push_back(n);
			}
			else if (type == "vt") {
				if (isDebugActive) std::cout << "Line type: VT" << std::endl;
				converter::texture t;
				t.x = std::stof(tokens[1]);
				t.y = std::stof(tokens[2]);
				converter::print::texture(t);
				textures->push_back(t);
			}
			else if (type == "f") {
				converter::face face;
				std::vector<std::string> subtokens1 = converter::split(tokens[1], "/");
				face.v1 = std::stoi(subtokens1[0]) - 1;
				face.t1 = (subtokens1[1] == "")? -1.0f : std::stoi(subtokens1[1]) - 1;
				face.n1 = std::stoi(subtokens1[2]) - 1;

				std::vector<std::string> subtokens2 = converter::split(tokens[2], "/");
				face.v2 = std::stoi(subtokens2[0]) - 1;
				face.t2 = (subtokens2[1] == "") ? -1 : std::stoi(subtokens2[1]) - 1;
				face.n2 = std::stoi(subtokens2[2]) - 1;

				std::vector<std::string> subtokens3 = converter::split(tokens[3], "/");
				face.v3 = std::stoi(subtokens3[0]) - 1;
				face.t3 = (subtokens3[1] == "") ? -1 : std::stoi(subtokens3[1]) - 1;
				face.n3 = std::stoi(subtokens3[2]) - 1;

				std::vector<std::string> subtokens4 = converter::split(tokens[4], "/");
				face.v4 = std::stoi(subtokens4[0]) - 1;
				face.t4 = (subtokens4[1] == "") ? -1 : std::stoi(subtokens4[1]) - 1;
				face.n4 = std::stoi(subtokens4[2]) - 1;

				converter::print::face(face);
				faces->push_back(face);
			}
			line_no++;
		}
	}

	void flattenToArrays(std::vector<converter::vertex>* vertexes, std::vector<converter::normal>* normals, std::vector<converter::face>* faces, std::vector<converter::texture>* textures, std::vector<float>* vertexesFlatten, std::vector<float>* normalsFlatten, std::vector<float>* texturesFlatten) {
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
			converter::face face = faces->at(i);
			converter::print::face(face);
			// upper triangle
			// vertices
			converter::flatten(vertexesFlatten, vertexes->at(face.v4));
			if(isDebugActive) std::cout << "Tailing vertexesFlatten: ";
			printg::last(vertexesFlatten, 4);
			converter::flatten(vertexesFlatten, vertexes->at(face.v2));
			if (isDebugActive) std::cout << "Tailing vertexesFlatten: ";
			printg::last(vertexesFlatten, 4);
			converter::flatten(vertexesFlatten, vertexes->at(face.v1));
			if (isDebugActive) std::cout << "Tailing vertexesFlatten: ";
			printg::last(vertexesFlatten, 4);

			// textures
			if (face.t1 != -1 && face.t2 != -1 && face.t3 != -1) {
				converter::flatten(texturesFlatten, textures->at(face.t4));
				if (isDebugActive) std::cout << "Tailing texturesFlatten: ";
				printg::last(texturesFlatten, 2);
				converter::flatten(texturesFlatten, textures->at(face.t2));
				if (isDebugActive) std::cout << "Tailing texturesFlatten: ";
				printg::last(texturesFlatten, 2);
				converter::flatten(texturesFlatten, textures->at(face.t1));
				if (isDebugActive) std::cout << "Tailing texturesFlatten: ";
				printg::last(texturesFlatten, 2);
			}
			else {
				for (int j = 0; j < 6; j++) texturesFlatten->push_back(0);
			}

			// normals
			converter::flatten(normalsFlatten, normals->at(face.n4));
			if (isDebugActive) std::cout << "Tailing normalsFlatten: ";
			printg::last(normalsFlatten, 4);
			converter::flatten(normalsFlatten, normals->at(face.n2));
			if (isDebugActive) std::cout << "Tailing normalsFlatten: ";
			printg::last(normalsFlatten, 4);
			converter::flatten(normalsFlatten, normals->at(face.n1));
			if (isDebugActive) std::cout << "Tailing normalsFlatten: ";
			printg::last(normalsFlatten, 4);

			// bottom triangle
			// vertices
			converter::flatten(vertexesFlatten, vertexes->at(face.v4));
			if (isDebugActive) std::cout << "Tailing vertexesFlatten: ";
			printg::last(vertexesFlatten, 4);
			converter::flatten(vertexesFlatten, vertexes->at(face.v3));
			if (isDebugActive) std::cout << "Tailing vertexesFlatten: ";
			printg::last(vertexesFlatten, 4);
			converter::flatten(vertexesFlatten, vertexes->at(face.v2));
			if (isDebugActive) std::cout << "Tailing vertexesFlatten: ";
			printg::last(vertexesFlatten, 4);

			//textures
			if (face.t2 != -1 && face.t3 != -1 && face.t4 != -1) {
				converter::flatten(texturesFlatten, textures->at(face.t4));
				if (isDebugActive) std::cout << "Tailing texturesFlatten: ";
				printg::last(texturesFlatten, 2);
				converter::flatten(texturesFlatten, textures->at(face.t3));
				if (isDebugActive) std::cout << "Tailing texturesFlatten: ";
				printg::last(texturesFlatten, 2);
				converter::flatten(texturesFlatten, textures->at(face.t2));
				if (isDebugActive) std::cout << "Tailing texturesFlatten: ";
				printg::last(texturesFlatten, 2);
			}
			else {
				for (int j = 0; j < 6; j++) texturesFlatten->push_back(0);
			}

			// normals
			converter::flatten(normalsFlatten, normals->at(face.n4));
			if (isDebugActive) std::cout << "Tailing normalsFlatten: ";
			printg::last(normalsFlatten, 4);
			converter::flatten(normalsFlatten, normals->at(face.n3));
			if (isDebugActive) std::cout << "Tailing normalsFlatten: ";
			printg::last(normalsFlatten, 4);
			converter::flatten(normalsFlatten, normals->at(face.n2));
			if (isDebugActive)std::cout << "Tailing normalsFlatten: ";
			printg::last(normalsFlatten, 4);
		}

	}

	void createFile(std::string filename, std::string namespacee, std::vector<float> vertexes, std::vector<float> normals, std::vector<float> textures) {
		std::ofstream file(filename.append(".h"), std::ios::trunc);
		file << "// auto generated for " << filename << std::endl;
		file << "namespace " << namespacee << " {" << std::endl;

		file << "	" << "const int vertexCount = " << vertexes.size() / 4 << ";" << std::endl;

		file << "	float vertices[] = {" << std::endl;
		for (int i = 0; i < vertexes.size() - 1; i++) {
			file << "		" << vertexes[i] << ",";
			if ((i + 1) % 4 == 0) file << std::endl;
		}
		file << vertexes[vertexes.size() - 1] << std::endl;
		file << "	};" << std::endl;

		file << "	float textures[] = {" << std::endl;
		for (int i = 0; i < textures.size() - 1; i++) {
			file << "		" << textures[i] << ",";
			if ((i + 1) % 2 == 0) file << std::endl;
		}
		file << textures[textures.size() - 1] << std::endl;
		file << "	};" << std::endl;

		file << "	float normals[] = {" << std::endl;
		for (int i = 0; i < normals.size() - 1; i++) {
			file << "		" << normals[i] << ",";
			if ((i + 1) % 4 == 0) file << std::endl;
		}
		file << normals[normals.size() - 1] << std::endl;
		file << "	};" << std::endl;
		file << "}";

		file.close();
	}

	void convert(std::string filename, std::string namespacee) {

		std::cout << "Reading model: " << filename << std::endl;
		std::vector<converter::vertex> vertexes;
		std::vector<converter::normal> normals;
		std::vector<converter::face> faces;
		std::vector<converter::texture> textures;

		std::vector<float> vertexesResult;
		std::vector<float> normalsResult;
		std::vector<float> texturesResult;

		converter::inflateToStructs(&vertexes, &normals, &faces, &textures, filename);
		converter::flattenToArrays(&vertexes, &normals, &faces, &textures, &vertexesResult, &normalsResult, &texturesResult);
		converter::createFile(filename, namespacee, vertexesResult, normalsResult, texturesResult);
	}
}

namespace updater {
	namespace general {
		void createAndOpen(std::ifstream* file, objects::Figure figure) {
			std::fstream f(objects::controllFilePath[figure], std::ios::out | std::ios::trunc);
			f << "position: 0 0 0" << std::endl;
			f << "scale: 1 1 1" << std::endl;
			f << "rotation: 0 0 0" << std::endl;
			f << "ambient: 0 0 0" << std::endl;
			f << "roughness: 0" << std::endl;
			f << "metallic: 0" << std::endl;
			f.close();

			file->open(objects::controllFilePath[figure]);
		}

		void createAndOpen(std::ifstream* file, std::string path) {
			std::fstream f(path, std::ios::out | std::ios::trunc);
			f << "light1Pos: 0 0 0" << std::endl;
			f << "light2Pos: 0 0 0" << std::endl;
			f.close();

			file->open(path);
		}
	}

	namespace prop {
		void updateScale(float x, float y, float z, glm::mat4* M) {
			transformationsInject::scale(M, transformations::Axis::x, x);
			transformationsInject::scale(M, transformations::Axis::y, y);
			transformationsInject::scale(M, transformations::Axis::z, z);
		}

		void updatePosition(float x, float y, float z, glm::mat4* M) {
			transformationsInject::move(M, transformations::Axis::x, x);
			transformationsInject::move(M, transformations::Axis::y, y);
			transformationsInject::move(M, transformations::Axis::z, z);
		}

		void updateRotation(float x, float y, float z, glm::mat4* M) {
			transformationsInject::rotate(M, transformations::Axis::x, glm::radians(x));
			transformationsInject::rotate(M, transformations::Axis::y, glm::radians(y));
			transformationsInject::rotate(M, transformations::Axis::z, glm::radians(z));
		}

		void  setAmbientOcculusion(float x, float y, float z, objects::Figure figure) {
			glm::vec3 ao = glm::vec3(x, y, z);
			objects::ambientOcculusion[figure] = ao;
		}

		void update(objects::Figure figure) {
			if (!makeAutoUpdate) return;
			std::ifstream file(objects::controllFilePath[figure]);
			glm::mat4 M = glm::mat4(1.0f);

			if (!file) updater::general::createAndOpen(&file, figure);

			while (!file.eof()) {
				std::string line;

				std::getline(file, line);
				std::vector<std::string> tokens = converter::split(line, " ");
				std::string selector = tokens[0];
				printg::vector(tokens);
				if (selector == "position:") updater::prop::updatePosition(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]), &M);
				else if (selector == "rotation:") updater::prop::updateRotation(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]), &M);
				else if (selector == "scale:") updater::prop::updateScale(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]), &M);
				else if (selector == "metallic:") objects::metallic[figure] = std::stof(tokens[1]);
				else if (selector == "ambient:") updater::prop::setAmbientOcculusion(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]), figure);
				else if (selector == "roughness:") objects::roughness[figure] = std::stof(tokens[1]);
			}
			objects::M[figure] = M;
			file.close();
		}
	}

	namespace settings {
		void setLight1Pos(float x, float y, float z) {
			global::light1Pos = glm::vec3(x, y, z);
		}

		void setLight2Pos(float x, float y, float z) {
			global::light2Pos = glm::vec3(x, y, z);
		}

		void update() {
			if (!makeAutoUpdate) return;
			std::ifstream file(global::settingsPath);

			if (!file) updater::general::createAndOpen(&file, global::settingsPath);
			while (!file.eof()) {
				std::string line;
				getline(file, line);
				std::vector<std::string> tokens = converter::split(line, " ");
				std::string selector = tokens[0];

				printg::vector(tokens);

				if (selector == "light1Pos:") updater::settings::setLight1Pos(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
				else if (selector == "light2Pos:") updater::settings::setLight2Pos(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
			}
			file.close();
		}
	}

	namespace texture {
		void update(objects::Figure figure) {
			objects::textureMap[figure] = render::readTexture(objects::imagePathA[figure], objects::textureUnitA[figure]);
			objects::specularMap[figure] = render::readTexture(objects::imagePathB[figure], objects::textureUnitB[figure]);
		}

		void updateAll() {
			updater::texture::update(objects::Figure::board);
			updater::texture::update(objects::Figure::bishop_w1);
		}
	}

	namespace shader {
		void update() {
			render::shaderProgram = new ShaderProgram("shaders/v_ct.glsl", NULL, "shaders/f_ct.glsl");
		}
	}
}

namespace board {
	const objects::Figure figure = objects::Figure::board;

	void init() {
		objects::M[board::figure] = glm::mat4(1.0f);
		objects::controllFilePath[board::figure] = "models/board/controll.prop";
		objects::imagePathA[board::figure] = "models/board/texture.png";
		objects::imagePathB[board::figure] = "models/board/specular.png";
		objects::vertices[board::figure] = board_obj::vertices;
		objects::texCoords[board::figure] = board_obj::textures;
		objects::normals[board::figure] = board_obj::normals;
		objects::vertexCount[board::figure] = board_obj::vertexCount;
		objects::textureUnitA[board::figure] = GL_TEXTURE0;
		objects::textureUnitB[board::figure] = GL_TEXTURE1;
		objects::textureUnitNumberA[board::figure] = 0;
		objects::textureUnitNumberB[board::figure] = 1;
		objects::textureMap[board::figure] = render::readTextureA(board::figure);
		objects::specularMap[board::figure] = render::readTextureB(board::figure);
		objects::ambientOcculusion[board::figure] = glm::vec3(1, 1, 1);
		objects::roughness[board::figure] = 0;
		objects::metallic[board::figure] = 0.5;
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
		updater::prop::update(board::figure);
		render::render(board::figure);
		// std::cout << figure;
	}

}

namespace bishop_w1{
	const objects::Figure figure = objects::Figure::bishop_w1;

	void init() {
		objects::M[bishop_w1::figure] = glm::mat4(1.0f);
		objects::controllFilePath[bishop_w1::figure] = "models/bishop_w/controll.prop";
		objects::imagePathA[bishop_w1::figure] = "models/bishop_w/texture.png";
		objects::imagePathB[bishop_w1::figure] = "models/bishop_w/specular.png";
		objects::vertices[bishop_w1::figure] = bishow_w_obj::vertices;
		objects::texCoords[bishop_w1::figure] = bishow_w_obj::textures;
		objects::normals[bishop_w1::figure] = bishow_w_obj::normals;
		objects::vertexCount[bishop_w1::figure] = bishow_w_obj::vertexCount;
		objects::textureUnitA[bishop_w1::figure] = GL_TEXTURE2;
		objects::textureUnitB[bishop_w1::figure] = GL_TEXTURE3;
		objects::textureUnitNumberA[bishop_w1::figure] = 2;
		objects::textureUnitNumberB[bishop_w1::figure] = 3;
		objects::textureMap[bishop_w1::figure] = render::readTextureA(bishop_w1::figure);
		objects::specularMap[bishop_w1::figure] = render::readTextureB(bishop_w1::figure);
		objects::ambientOcculusion[bishop_w1::figure] = glm::vec3(1, 1, 1);
		objects::roughness[bishop_w1::figure] = 0;
		objects::metallic[bishop_w1::figure] = 0.5;

		// for (int i = 0; i < 36; i++) std::cout << objects::vertices[figure][i] << " ";
	}

	void move(float distance, transformations::Axis axis) {
		transformations::move(bishop_w1::figure, axis, distance);
	}

	void rotate(float angle, transformations::Axis axis) {
		transformations::rotate(bishop_w1::figure, axis, angle);
	}

	void scale(float scale, transformations::Axis axis) {
		transformations::scale(bishop_w1::figure, axis, scale);
	}

	void render() {
		updater::prop::update(bishop_w1::figure);
		render::render(bishop_w1::figure);
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
		if (key == GLFW_KEY_E) updater::texture::updateAll();
		if (key = GLFW_KEY_Q) updater::shader::update();
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
	bishop_w1::init();
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
	updater::settings::update();
	render::start();
	observer::rotate();
	observer::move();
	board::render();
	bishop_w1::render();
	debug::printAll();
    glfwSwapBuffers(window); //Przerzuć tylny bufor na przedni
}

void test() {
	converter::convert("models/bishop_b/object.obj", "bishop_b_obj");
	// converter::convert("models/rook_w/object.obj", "rook_w_obj");
}

void mainline() {
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

int main(void)
{
	// mainline();
	test();
}

