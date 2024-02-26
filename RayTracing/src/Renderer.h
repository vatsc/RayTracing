#pragma once
#define ut uint32_t
#include "Walnut/Image.h"
#include "Camera.h"
#include "Ray.h"
#include "Scene.h"
#include <memory>
#include <glm/glm.hpp>
class Renderer
{
public:
	Renderer() = default;
	void OnResize(ut width, ut height);
	void Render(const Scene& scene,const Camera& camera);
	
	std::shared_ptr<Walnut::Image> GetFinalImage() const
	{
		return m_FinalImage;
	}
private:
	glm::vec4 TraceRay(const Scene& scene,const Ray& coord);
private:
	std::shared_ptr<Walnut::Image> m_FinalImage;
	uint32_t* m_ImageData = nullptr;
};