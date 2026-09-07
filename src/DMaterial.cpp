#include "DMaterial.h"

DMaterial::DMaterial()
{

}

void DMaterial::init(std::vector<std::string> textures)
{
	for (const auto& texture : textures)
	{
		texturePaths.push_back(texture);
	}
}
