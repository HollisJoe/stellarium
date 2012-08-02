#include "StelGeometryBuilder.hpp"

#include "StelRenderer.hpp"


//TODO These lookup tables and their generation function need documentation

static const int MAX_SLICES = 4096;
static float COS_SIN_THETA[2 * (MAX_SLICES + 1)];
static const int MAX_STACKS = 4096;
static float COS_SIN_RHO[2 * (MAX_STACKS + 1)];

static void computeCosSinTheta(const float phi, const int segments)
{
	float *cosSin        = COS_SIN_THETA;
	float *cosSinReverse = cosSin + 2 * (segments + 1);

	const float c = std::cos(phi);
	const float s = std::sin(phi);

	*cosSin++ = 1.0f;
	*cosSin++ = 0.0f;
	*--cosSinReverse = -cosSin[-1];
	*--cosSinReverse =  cosSin[-2];
	*cosSin++ = c;
	*cosSin++ = s;
	*--cosSinReverse = -cosSin[-1];
	*--cosSinReverse =  cosSin[-2];

	while (cosSin < cosSinReverse)
	{
		cosSin[0] = cosSin[-2] * c - cosSin[-1] * s;
		cosSin[1] = cosSin[-2] * s + cosSin[-1] * c;
		cosSin += 2;
		*--cosSinReverse = -cosSin[-1];
		*--cosSinReverse =  cosSin[-2];
	}
}

static void computeCosSinRho(const float phi, const int segments)
{
	float *cosSin        = COS_SIN_RHO;
	float *cosSinReverse = cosSin + 2 * (segments + 1);

	const float c = std::cos(phi);
	const float s = std::sin(phi);

	*cosSin++ = 1.0f;
	*cosSin++ = 0.0f;

	*--cosSinReverse =  cosSin[-1];
	*--cosSinReverse = -cosSin[-2];

	*cosSin++ = c;
	*cosSin++ = s;

	*--cosSinReverse =  cosSin[-1];
	*--cosSinReverse = -cosSin[-2];

	while (cosSin < cosSinReverse)
	{
		cosSin[0] = cosSin[-2] * c - cosSin[-1] * s;
		cosSin[1] = cosSin[-2] * s + cosSin[-1] * c;
		cosSin += 2;
		*--cosSinReverse =  cosSin[-1];
		*--cosSinReverse = -cosSin[-2];
	}
}

void StelGeometrySphere::draw(StelRenderer* renderer, StelProjectorP projector)
{
	// Lit spheres must update at every projector as projector is used in lighting.
	if(updated || type == SphereType_Lit)
	{
		regenerate(renderer, projector);
	}

	switch(type)
	{
		case SphereType_Fisheye:
		case SphereType_Unlit:
		{
			for(int row = 0; row < rowIndices.size(); ++row)
			{
				renderer->drawVertexBuffer(unlitVertices, rowIndices[row], projector);
			}
			break;
		}
		case SphereType_Lit:
		{
			for(int row = 0; row < rowIndices.size(); ++row)
			{
				renderer->drawVertexBuffer(litVertices, rowIndices[row], projector);
			}
			break;
		}
		default:
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown sphere type");
	}
} 

void StelGeometrySphere::regenerate(class StelRenderer* renderer, StelProjectorP projector)
{
#ifndef NDEBUG
	switch(type)
	{
		case SphereType_Fisheye:
		case SphereType_Unlit:
		case SphereType_Lit:
			break;
		default:
			Q_ASSERT_X(false, Q_FUNC_INFO, 
			           "New sphere type - need to update StelGeometrySphere::regenerate()");
	}
#endif

	// Prepare the vertex buffer
	if(type == SphereType_Fisheye || type == SphereType_Unlit)
	{
		Q_ASSERT_X(litVertices == NULL, Q_FUNC_INFO,
		           "Lit vertex buffer is used for unlit sphere");
		if(unlitVertices == NULL)
		{
			unlitVertices = 
				renderer->createVertexBuffer<VertexP3T2>(PrimitiveType_TriangleStrip);
		}
		else
		{
			unlitVertices->unlock();
			unlitVertices->clear();
		}
	}
	else if(type == SphereType_Lit)
	{
		Q_ASSERT_X(unlitVertices == NULL, Q_FUNC_INFO,
		           "Unlit vertex buffer is used for lit sphere");
		if(litVertices == NULL)
		{
			litVertices = 
				renderer->createVertexBuffer<VertexP3T2C4>(PrimitiveType_TriangleStrip);
		}
		else
		{
			litVertices->unlock();
			litVertices->clear();
		}
	}
	else
	{
		Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown sphere type");
	}

	// Prepare index buffers for rows.
	// Add/remove rows based on stacks. Clear reused rows.
	const int rowsKept = std::min(stacks, rowIndices.size());
	for(int row = 0; row < rowsKept; ++row)
	{
		rowIndices[row]->unlock();
		rowIndices[row]->clear();
	}

	if(stacks > rowIndices.size())
	{
		for(int row = rowIndices.size(); row < stacks; ++row)
		{
			rowIndices.append(renderer->createIndexBuffer(IndexType_U16));
		}
	}
	else if(stacks < rowIndices.size())
	{
		for(int row = stacks; row < rowIndices.size(); ++row)
		{
			delete rowIndices[row];
		}
		rowIndices.resize(stacks);
	}


	// Now actually build the sphere

	const float dtheta     = 2.0f * M_PI / slices;
	computeCosSinTheta(dtheta, slices);

	if(type == SphereType_Fisheye)
	{
		StelVertexBuffer<VertexP3T2>* vertices = unlitVertices;
		const float stackAngle = M_PI / stacks;
		const float drho       = stackAngle / fisheyeTextureFov;
		computeCosSinRho(stackAngle, stacks);

		const Vec2f texOffset(0.5f, 0.5f);
		float yTexMult = orientInside ? -1.0f : 1.0f;
		float xTexMult = flipTexture ? -1.0f : 1.0f;

		// Build intermediate stacks as triangle strips
		const float* cosSinRho = COS_SIN_RHO;
		float rho = 0.0f;

		// Generate all vertices of the sphere.
		vertices->unlock();
		for (int i = 0; i <= stacks; ++i, rho += drho)
		{
			const float rhoClamped = std::min(rho, 0.5f);
			const float cosSinRho0 = cosSinRho[0];
			const float cosSinRho1 = cosSinRho[1];
			
			const float* cosSinTheta = COS_SIN_THETA;
			for (int slice = 0; slice <= slices; ++slice)
			{
				const Vec3f v(-cosSinTheta[1] * cosSinRho1,
								  cosSinTheta[0] * cosSinRho1, 
								  cosSinRho0 * oneMinusOblateness);
				const Vec2f t(xTexMult * cosSinTheta[0], yTexMult * cosSinTheta[1]);
				vertices->addVertex(VertexP3T2(v * radius, texOffset + rhoClamped * t));
				cosSinTheta += 2;
			}

			cosSinRho += 2;
		}
		vertices->lock();

		// Generate index buffers for strips (rows) forming the sphere.
		uint index = 0;
		for (int i = 0; i < stacks; ++i)
		{
			StelIndexBuffer* const indices = rowIndices[i];
			indices->unlock();

			for (int slice = 0; slice <= slices; ++slice)
			{
				const uint i1 = orientInside ? index + slices + 1 : index;
				const uint i2 = orientInside ? index : index + slices + 1;
				indices->addIndex(i1);
				indices->addIndex(i2);
				index += 1;
			}

			indices->lock();
		}
	}
	else if(type == SphereType_Unlit)
	{ 
		StelVertexBuffer<VertexP3T2>* vertices = unlitVertices;
		vertices->unlock();
		const float drho   = M_PI / stacks;
		computeCosSinRho(drho, stacks);

		const float nsign = orientInside ? -1.0f : 1.0f;
		// from inside texture is reversed 
		float t           = orientInside ?  0.0f : 1.0f;

		// texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
		// t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
		// cannot use triangle fan on texturing (s coord. at top/bottom tip varies)
		const float ds = (flipTexture ? -1.0f : 1.0f) / slices;
		const float dt = nsign / stacks; // from inside texture is reversed

		const float* cosSinRho = COS_SIN_RHO;
		for (int i = 0; i <= stacks; ++i)
		{
			const float cosSinRho0 = cosSinRho[0];
			const float cosSinRho1 = cosSinRho[1];
			
			float s = flipTexture ? 1.0f : 0.0f;
			const float* cosSinTheta = COS_SIN_THETA;
			for (int slice = 0; slice <= slices; ++slice)
			{
				const Vec3f v(-cosSinTheta[1] * cosSinRho1,
				              cosSinTheta[0] * cosSinRho1, 
				              nsign * cosSinRho0 * oneMinusOblateness);
				vertices->addVertex(VertexP3T2(v * radius, Vec2f(s, t)));
				s += ds;
				cosSinTheta += 2;
			}

			cosSinRho += 2;
			t -= dt;
		}
		// Generate index buffers for strips (rows) forming the sphere.
		uint index = 0;
		for (int i = 0; i < stacks; ++i)
		{
			StelIndexBuffer* const indices = rowIndices[i];
			indices->unlock();

			for (int slice = 0; slice <= slices; ++slice)
			{
				indices->addIndex(index);
				indices->addIndex(index + slices + 1);
				index += 1;
			}

			indices->lock();
		}

		vertices->lock(); 
	}
	else if(type == SphereType_Lit)
	{
		StelVertexBuffer<VertexP3T2C4>* vertices = litVertices;
		// Set up the light.
		Vec3d lightPos = Vec3d(light.position[0], light.position[1], light.position[2]);
		projector->getModelViewTransform()
		         ->getApproximateLinearTransfo()
		         .transpose()
		         .multiplyWithoutTranslation(Vec3d(lightPos[0], lightPos[1], lightPos[2]));
		projector->getModelViewTransform()->backward(lightPos);
		lightPos.normalize();
		const Vec4f ambientLight = light.ambient;
		const Vec4f diffuseLight = light.diffuse; 

		// Set up vertex generation.
		vertices->unlock();
		const float drho   = M_PI / stacks;
		computeCosSinRho(drho, stacks);

		const float nsign = orientInside ? -1.0f : 1.0f;
		// from inside texture is reversed 
		float t           = orientInside ?  0.0f : 1.0f;

		// texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
		// t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
		// cannot use triangle fan on texturing (s coord. at top/bottom tip varies)
		const float ds = (flipTexture ? -1.0f : 1.0f) / slices;
		const float dt = nsign / stacks; // from inside texture is reversed

		// Generate sphere vertices with lighting baked in colors.
		const float* cosSinRho = COS_SIN_RHO;
		for (int i = 0; i <= stacks; ++i)
		{
			const float cosSinRho0 = cosSinRho[0];
			const float cosSinRho1 = cosSinRho[1];
			
			float s = flipTexture ? 1.0f : 0.0f;
			const float* cosSinTheta = COS_SIN_THETA;
			for (int slice = 0; slice <= slices; ++slice)
			{
				const Vec3f v(-cosSinTheta[1] * cosSinRho1,
								  cosSinTheta[0] * cosSinRho1, 
								  nsign * cosSinRho0 * oneMinusOblateness);
				const float c = 
					std::max(0.0, nsign * (lightPos[0] * v[0] * oneMinusOblateness +
												  lightPos[1] * v[1] * oneMinusOblateness +
												  lightPos[2] * v[2] / oneMinusOblateness));
				const Vec4f color = std::min(c, 0.5f) * diffuseLight + ambientLight;
				vertices->addVertex(VertexP3T2C4(v * radius, Vec2f(s, t), color));
				s += ds;
				cosSinTheta += 2;
			}

			cosSinRho += 2;
			t -= dt;
		}
		// Generate index buffers for strips (rows) forming the sphere.
		uint index = 0;
		for (int i = 0; i < stacks; ++i)
		{
			StelIndexBuffer* const indices = rowIndices[i];
			indices->unlock();

			for (int slice = 0; slice <= slices; ++slice)
			{
				indices->addIndex(index);
				indices->addIndex(index + slices + 1);
				index += 1;
			}

			indices->lock();
		}

		vertices->lock(); 
	}
	else
	{
		Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown sphere type");
	}

	updated = false;
}

void StelGeometryBuilder::buildFanDisk
	(StelVertexBuffer<VertexP3T2>* const vertexBuffer, StelIndexBuffer* const indexBuffer,
	 const float radius, const int innerFanSlices, const int level)
{
	Q_ASSERT_X(vertexBuffer->length() == 0, Q_FUNC_INFO, 
	           "Need an empty vertex buffer to start building a fan disk");
	Q_ASSERT_X(indexBuffer->length() == 0, Q_FUNC_INFO, 
	           "Need an empty index buffer to start building a fan disk");
	Q_ASSERT_X(vertexBuffer->primitiveType() == PrimitiveType_Triangles, 
	           Q_FUNC_INFO, "Need a triangles vertex buffer to build a fan disk");
	Q_ASSERT_X(innerFanSlices >= 3, Q_FUNC_INFO, 
	           "Can't build a fan disk with less than 3 slices");
	Q_ASSERT_X(level < 32, Q_FUNC_INFO, 
	           "Can't build a fan disk with more than 31 levels "
	           "(to prevent excessive vertex counts - this limit can be increased)");

	vertexBuffer->unlock();
	indexBuffer->unlock();
	float radii[32];
	radii[level] = radius;
	for (int l = level - 1; l >= 0; --l)
	{
		radii[l] = radii[l + 1] * 
		           (1.0f - M_PI / (innerFanSlices << (l + 1))) * 2.0f / 3.0f;
	}

	const int slices = innerFanSlices << level;
	const float dtheta = 2.0f * M_PI / slices;
	Q_ASSERT_X(slices <= MAX_SLICES, Q_FUNC_INFO, "Too many slices");
	computeCosSinTheta(dtheta, slices);

	int slicesStep = 2;
	
	// Texcoords at the center are 0.5, 0.5, and vary between 0.0 to 1.0 for 
	// opposite sides in distance of radius from center.
	const float texMult = 0.5 / radius;
	const Vec2f texOffset(0.5f, 0.5f);

	// Current index in the index buffer.
	uint index = 0;

	for (int l = level; l > 0; --l, slicesStep <<= 1)
	{
		const float* cosSinTheta = COS_SIN_THETA;
		for (int s = 0; s < slices - 1; s += slicesStep)
		{
			const Vec2f v0(radii[l] * cosSinTheta[slicesStep],
			               radii[l] * cosSinTheta[slicesStep + 1]);
			const Vec2f v1(radii[l] * cosSinTheta[2 * slicesStep],
			               radii[l] * cosSinTheta[2 * slicesStep +1]);
			const Vec2f v2(radii[l - 1] * cosSinTheta[2 * slicesStep],
			               radii[l - 1] * cosSinTheta[2 * slicesStep + 1]);
			const Vec2f v3(radii[l - 1] * cosSinTheta[0], radii[l - 1] * cosSinTheta[1]);
			const Vec2f v4(radii[l] * cosSinTheta[0], radii[l] * cosSinTheta[1]);

			vertexBuffer->addVertex(VertexP3T2(v0, texOffset + v0 * texMult));
			vertexBuffer->addVertex(VertexP3T2(v1, texOffset + v1 * texMult));
			vertexBuffer->addVertex(VertexP3T2(v2, texOffset + v2 * texMult));
			vertexBuffer->addVertex(VertexP3T2(v3, texOffset + v3 * texMult));
			vertexBuffer->addVertex(VertexP3T2(v4, texOffset + v4 * texMult));

			indexBuffer->addIndex(index);
			indexBuffer->addIndex(index + 1);
			indexBuffer->addIndex(index + 2);
			indexBuffer->addIndex(index);
			indexBuffer->addIndex(index + 2);
			indexBuffer->addIndex(index + 3);
			indexBuffer->addIndex(index);
			indexBuffer->addIndex(index + 3);
			indexBuffer->addIndex(index + 4);

			cosSinTheta += 2 * slicesStep;
			index += 5;
		}
	}

	// draw the inner polygon
	slicesStep >>= 1;

	const float* cosSinTheta = COS_SIN_THETA;
	const float rad = radii[0];
	if (slices == 1)
	{
		const Vec2f v0(rad * cosSinTheta[0], rad * cosSinTheta[1]);
		cosSinTheta += 2 * slicesStep;
		const Vec2f v1(rad * cosSinTheta[0], rad * cosSinTheta[1]);
		cosSinTheta += 2 * slicesStep;
		const Vec2f v2(rad * cosSinTheta[0], rad * cosSinTheta[1]);

		vertexBuffer->addVertex(VertexP3T2(v0, texOffset + v0 * texMult));
		vertexBuffer->addVertex(VertexP3T2(v1, texOffset + v1 * texMult));
		vertexBuffer->addVertex(VertexP3T2(v2, texOffset + v2 * texMult));

		indexBuffer->addIndex(index++);
		indexBuffer->addIndex(index++);
		indexBuffer->addIndex(index++);

		vertexBuffer->lock();
		indexBuffer->lock();
		return;
	}

	int s = 0;
	while (s < slices)
	{
		const Vec2f v1(rad * cosSinTheta[0], rad * cosSinTheta[1]);
		cosSinTheta += 2 * slicesStep;
		const Vec2f v2(rad * cosSinTheta[0], rad * cosSinTheta[1]);

		vertexBuffer->addVertex(VertexP3T2(Vec3f(0.0f, 0.0f, 0.0f), texOffset));
		vertexBuffer->addVertex(VertexP3T2(v1, texOffset + v1 * texMult));
		vertexBuffer->addVertex(VertexP3T2(v2, texOffset + v2 * texMult));

		indexBuffer->addIndex(index++);
		indexBuffer->addIndex(index++);
		indexBuffer->addIndex(index++);

		s += slicesStep;
	}

	vertexBuffer->lock();
	indexBuffer->lock();
}

void StelGeometryBuilder::buildRing
(StelVertexBuffer<VertexP3T2>* vertices, QVector<StelIndexBuffer*>& rowIndexBuffers,
 const float rMin, const float rMax, int slices, bool flipFaces)
{
	const int stacks = rowIndexBuffers.size();
	Q_ASSERT_X(stacks > 0, Q_FUNC_INFO, "Need at least 1 row buffer to build a ring");
	Q_ASSERT_X(slices > 3, Q_FUNC_INFO, "Need at least 3 slices to build a ring");
	Q_ASSERT_X(vertices->primitiveType() == PrimitiveType_TriangleStrip, Q_FUNC_INFO,
	           "Need a triangle strip vertex buffer to build a ring");
	Q_ASSERT_X(vertices->length() == 0, Q_FUNC_INFO, 
	           "Need an empty vertex buffer to build a ring");
	Q_ASSERT_X(rMin >= 0.0f, Q_FUNC_INFO, "Ring can't have a negative radius");
	Q_ASSERT_X(rMax > rMin, Q_FUNC_INFO, 
	           "Maximum ring radius must be greater than the minimum radius");

	const float dr     = (rMax - rMin) / stacks;
	const float dtheta = (flipFaces ? -1.0f : 1.0f) * 2.0f * M_PI / slices;
	Q_ASSERT_X(slices <= MAX_SLICES, Q_FUNC_INFO, "Too many slices");
	computeCosSinTheta(dtheta, slices);

	// Generate vertices of the ring.
	float r = rMin;
	for(int stack = 0; stack <= stacks; ++stack, r += dr)
	{
		const float texR = (r - rMin) / (rMax - rMin);
		const float* cosSinTheta = COS_SIN_THETA;
		for (int slice = 0; slice <= slices; ++slice, cosSinTheta += 2)
		{
			vertices->addVertex(VertexP3T2(Vec2f(r * cosSinTheta[0], r * cosSinTheta[1]),
			                               Vec2f(texR, 0.5f)));
		}
	}
	vertices->lock();

	// Generate a triangle strip index buffer for each stack.
	uint index = 0;
	for(int stack = 0; stack < stacks; ++stack)
	{
		StelIndexBuffer* indices = rowIndexBuffers[stack];
		Q_ASSERT_X(indices->length() == 0, Q_FUNC_INFO, 
		           "Need empty index buffers to build a ring");
		indices->unlock();
		for (int slice = 0; slice <= slices; ++slice)
		{
			indices->addIndex(index);
			indices->addIndex(index + slices + 1);
			++index;
		}
		indices->lock();
	}
}
