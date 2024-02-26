#include "Renderer.h"

#include "Walnut/Random.h"
namespace  Utils {

	static ut convertToRGBA(const glm::vec4 color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}
}
void Renderer::OnResize(ut width, ut height)
{
	if (m_FinalImage)
	{
		//No resize needed
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new ut[width * height];

}

void Renderer::Render(const Scene& scene,const Camera& camera)
{
	Ray ray;
	ray.Origin = camera.GetPosition();
	

	//render everry pixel!!!
	for (ut y = 0;y < m_FinalImage->GetHeight(); y++) {

		for (ut x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			//glm::vec2 coord = { (float)x / (float)m_FinalImage->GetWidth() , (float)y / (float)m_FinalImage->GetHeight() };
			//coord = coord * 2.0f - 1.0f; // -1 -> 1
			 ray.Direction = camera.GetRayDirections()[x + y * m_FinalImage->GetWidth()];

			glm::vec4 color = TraceRay(scene,ray);
			color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::convertToRGBA(color);
		}
	}
	m_FinalImage->SetData(m_ImageData);
}

glm::vec4 Renderer::TraceRay(const Scene& scene,const Ray& ray)
{
	//float rad = 0.5f;


	//Center of the sphere

	//rayDirection = glm::normalize(rayDirection);
	// 
	// (bx^2 + by^2)t^2 + 2*(axbx + ayby) + ax^2 + ay^2 - r^2 = 0
	//a = ray origin
	//b = ray dicrn
	//r = rad
	//t = hit distance

	if (scene.Spheres.size() == 0)
	{
		return glm::vec4(0, 0, 0, 1);
	}

	const Sphere* closestSphere = nullptr;
	float hitDistance = std::numeric_limits<float>::max();
	for (const Sphere& sphere : scene.Spheres)
	{
		glm::vec3 origin = ray.Origin - sphere.Position; // New origin after sphere has moved.


		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * (glm::dot(origin, ray.Direction));
		float c = glm::dot(origin, origin) - sphere.radius * sphere.radius;

		float D = b * b - 4 * a * c;

		if (D < 0)
		{
			continue;
		}
		//float t0 = (-b + glm::sqrt(D)) / (2.0f * a);
		float closestT = (-b - glm::sqrt(D)) / (2.0f * a);
		//closestSphere = &sphere;
		if (closestT < hitDistance)
		{
			hitDistance = closestT;
			closestSphere = &sphere;
		}
	}
	

	if (closestSphere == nullptr)
	{
		return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}


	//t1 < t0(always), thus the ray first hits at hitPosition1 on the sphere.
	glm::vec3 origin = ray.Origin - closestSphere->Position;
		glm::vec3 hitPosition1 = origin + ray.Direction * hitDistance;
		glm::vec3 PixelNormal = glm::normalize(hitPosition1);
		glm::vec3 lightDirection = glm::normalize(glm::vec3(-1, -1, -1));
		float Intensity = glm::max(glm::dot(PixelNormal, -lightDirection), 0.0f); // cos(theta), thetha being the angle between lightDirection and PixelNormal.

	glm::vec3 sphereColor = closestSphere->Albedo;
	sphereColor *=  Intensity;
	return glm::vec4(sphereColor, 1.0f);
}