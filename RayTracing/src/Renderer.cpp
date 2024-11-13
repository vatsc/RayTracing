#include "Renderer.h"

#include "Walnut/Random.h"
#include<execution>
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

	static uint32_t PCG_Hash(uint32_t input)
	{
		ut state = input * 747796405u + 2891336453u;
		ut word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return (word >> 22u) ^ word;
	}

	static float RandomFloat(uint32_t& seed)
	{
		seed = PCG_Hash(seed);
		return (float)seed / (float)UINT32_MAX;
	}

	static glm::vec3 InUnitSphere(uint32_t&seed)
	{
		return glm::normalize(glm::vec3(RandomFloat(seed) * 2.0f - 1.0f, RandomFloat(seed) * 2.0f - 1.0f, RandomFloat(seed) * 2.0f - 1.0f));
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

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];
	m_ImageHorizontalIter.resize(width);
	m_ImageVerticalIter.resize(height);

	for (uint32_t i = 0; i < width; i++)
	{
		m_ImageHorizontalIter[i] = i;
	}
	for (uint32_t i = 0; i < height; i++)
	{
		m_ImageVerticalIter[i] = i;
	}

}

void Renderer::Render(const Scene& scene,const Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	if (m_FrameIndex == 1)
	{
		memset(m_AccumulationData, 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));
	}
#define MT 1
#if MT
	std::for_each(std::execution::par,m_ImageVerticalIter.begin(), m_ImageVerticalIter.end(),
		[this](uint32_t y) {
			std::for_each(std::execution::par,m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(),
			[this,y](uint32_t x) {
					//PerPixel(x, y);
					//glm::vec2 coord = { (float)x / (float)m_FinalImage->GetWidth() , (float)y / (float)m_FinalImage->GetHeight() };
						//coord = coord * 2.0f - 1.0f; // -1 -> 1

					glm::vec4 color = PerPixel(x, y);
					m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;
					glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
					accumulatedColor /= (float)m_FrameIndex;
					accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
					m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::convertToRGBA(accumulatedColor);
				});
		});
#else
	//render everry pixel!!!
	for (ut y = 0;y < m_FinalImage->GetHeight(); y++) {

		for (ut x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			//PerPixel(x,y);
			//glm::vec2 coord = { (float)x / (float)m_FinalImage->GetWidth() , (float)y / (float)m_FinalImage->GetHeight() };
				//coord = coord * 2.0f - 1.0f; // -1 -> 1
				
			glm::vec4 color = PerPixel(x, y);
			m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;
			glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
			accumulatedColor /= (float)m_FrameIndex;
			accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::convertToRGBA(accumulatedColor);
		}
	}
#endif
	m_FinalImage->SetData(m_ImageData);

	if (m_Settings.Accumulate)
	{
		m_FrameIndex++;
	}
	else m_FrameIndex = 1;
}



Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
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

	

	int closestSphere = -1;
	float hitDistance = std::numeric_limits<float>::max();
	for (size_t i=0; i < m_ActiveScene->Spheres.size(); i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
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
		if (closestT > 0.0f && closestT < hitDistance)
		{
			hitDistance = closestT;
			closestSphere = (int)i;
		}
	}
	

	if (closestSphere < 0)
	{
		return Miss(ray);
	}

	return ClosestHit(ray, hitDistance, closestSphere);
	
}

glm::vec4 Renderer::PerPixel(uint32_t x,uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];


	glm::vec3 light(0.0f);
	glm::vec3 leftcolor(1.0f);

	uint32_t seed = x + y * m_FinalImage->GetWidth();
	seed *= m_FrameIndex;

	int bounces = 5;
	for (int i=0; i < bounces; i++)
	{
		seed += i;
		Renderer::HitPayload payload = TraceRay(ray);
		if (payload.HitDistance < 0.0f)
		{
			glm::vec3 skyColor = glm::vec3(0.6f, 0.7f, 0.9f);
			//light += skyColor * leftcolor;
			break;
		}
			

		//glm::vec3 lightDirection = glm::normalize(glm::vec3(-1, -1, -1));
		//float Intensity = glm::max(glm::dot(payload.WorldNormal, -lightDirection), 0.0f); // cos(theta), thetha being the angle between lightDirection and PixelNormal.

		const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
		const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];

		//glm::vec3 sphereColor = material.Albedo;
		//sphereColor *= Intensity;
		//light += material.Albedo *leftcolor;
		leftcolor *=material.Albedo;
		light += material.GetEmission();

		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;
		//ray.Direction = glm::reflect(ray.Direction, payload.WorldNormal + material.Roughness * Walnut::Random::Vec3(- 0.5f, 0.5f ));
		if(m_Settings.SlowRandom)
		ray.Direction = glm::normalize(Walnut::Random::InUnitSphere()+payload.WorldNormal);
		else
			ray.Direction = glm::normalize(Utils::InUnitSphere(seed) + payload.WorldNormal);

	}
	


	return glm::vec4(light, 1.0f);

}



Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex)
{
	//t1 < t0(always), thus the ray first hits at hitPosition1 on the sphere.

	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorldPosition);

	payload.WorldPosition += closestSphere.Position;

	return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.HitDistance = -1;
	return payload;
}
