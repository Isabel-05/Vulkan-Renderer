#pragma once
#include <string>
#include <vector>

class DMaterial
{
public:
	DMaterial();

	void init(std::vector<std::string> textures);

	std::string vertexShaderPath;
	std::string fragmentShaderPath;

	std::vector<std::string> texturePaths;

	bool isDirty = true;

private:

};