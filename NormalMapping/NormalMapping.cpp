#include "Framework.h"

#include "ShaderSet.h"
#include "Mesh.h"
#include "Texture.h"

//================================================================================
// Normal Mapping Application
// An example of how to work with normal maps.
//================================================================================
class NormalMappingApp : public FrameworkApp
{
public:

	struct PerFrameCBData
	{
		m4x4 m_matProjection;
		m4x4 m_matView;
		v4 m_lightPos;
		f32		m_time;
		f32     m_padding[3];
	};

	struct PerDrawCBData
	{
		m4x4 m_matMVP;
		m4x4 m_matWorld;
		v4 m_matNormal[3]; // because of structure packing rules this represents a float3x3 in HLSL.
	};

	void on_init(SystemsInterface& systems) override
	{
		m_position = v3(0.5f, 0.5f, 0.5f);
		m_size = 1.0f;
		systems.pCamera->eye = v3(10.f, 5.f, 7.f);
		systems.pCamera->look_at(v3(3.f, 0.5f, 0.f));

		// compile a set of shaders
		m_meshShader.init(systems.pD3DDevice
			, ShaderSetDesc::Create_VS_PS("Assets/Shaders/NormalMappingShaders.fx", "VS_Mesh", "PS_Mesh")
			, { VertexFormatTraits<MeshVertex>::desc, VertexFormatTraits<MeshVertex>::size }
		);

		// Create Per Frame Constant Buffer.
		m_pPerFrameCB = create_constant_buffer<PerFrameCBData>(systems.pD3DDevice);

		// Create Per Frame Constant Buffer.
		m_pPerDrawCB = create_constant_buffer<PerDrawCBData>(systems.pD3DDevice);

		// Initialize a mesh directly.
		create_mesh_cube(systems.pD3DDevice, m_meshArray[0], 0.5f);

		// Initialize a mesh from an .OBJ file
		create_mesh_from_obj(systems.pD3DDevice, m_meshArray[1], "Assets/Models/WoodCrate/wc1.obj", 1.f);

		// Initialise some textures;
		m_textures[0].init_from_dds(systems.pD3DDevice, "Assets/Models/WoodCrate/wc1_diffuse.dds");
		m_textures[1].init_from_dds(systems.pD3DDevice, "Assets/Models/WoodCrate/wc1_normal.dds");

		// We need a sampler state to define wrapping and mipmap parameters.
		m_pLinearMipSamplerState = create_basic_sampler(systems.pD3DDevice, D3D11_TEXTURE_ADDRESS_WRAP);

		// Setup per-frame data
		m_perFrameCBData.m_time = 0.0f;
	}

	void on_update(SystemsInterface& systems) override
	{
		//////////////////////////////////////////////////////////////////////////
		// You can use features from the ImGui library.
		// Investigate the ImGui::ShowDemoWindow() function for ideas.
		// see also : https://github.com/ocornut/imgui
		//////////////////////////////////////////////////////////////////////////

		// This function displays some useful debugging values, camera positions etc.
		DemoFeatures::editorHud(systems.pDebugDrawContext);

		ImGui::SliderFloat3("Position", (float*)&m_position, -1.f, 1.f);
		ImGui::SliderFloat("Size", &m_size, 0.1f, 10.f);

		// Update Per Frame Data.
		m_perFrameCBData.m_matProjection = systems.pCamera->projMatrix.Transpose();
		m_perFrameCBData.m_matView = systems.pCamera->viewMatrix.Transpose();
		m_perFrameCBData.m_time += 0.001f;
		m_perFrameCBData.m_lightPos = v4(sin(m_perFrameCBData.m_time*5.0f) * 4.f + 3.0f, 1.f, 1.f, 0.f);
	}

	void on_render(SystemsInterface& systems) override
	{
		//////////////////////////////////////////////////////////////////////////
		// Imgui can also be used inside the render function.
		//////////////////////////////////////////////////////////////////////////


		//////////////////////////////////////////////////////////////////////////
		// You can use features from the DebugDrawlibrary.
		// Investigate the following functions for ideas.
		// see also : https://github.com/glampert/debug-draw
		//////////////////////////////////////////////////////////////////////////

		// Grid from -50 to +50 in both X & Z
		auto ctx = systems.pDebugDrawContext;

		dd::xzSquareGrid(ctx, -50.0f, 50.0f, 0.0f, 1.f, dd::colors::DimGray);
		dd::axisTriad(ctx, (const float*)& m4x4::Identity, 0.1f, 15.0f);
		dd::cross(ctx, (const float*)& m_perFrameCBData.m_lightPos, 0.1f, 15.0f);

		// Push Per Frame Data to GPU
		push_constant_buffer(systems.pD3DContext, m_pPerFrameCB, m_perFrameCBData);

		// Bind our set of shaders.
		m_meshShader.bind(systems.pD3DContext);

		// Bind Constant Buffers, to both PS and VS stages
		ID3D11Buffer* buffers[] = { m_pPerFrameCB, m_pPerDrawCB };
		systems.pD3DContext->VSSetConstantBuffers(0, 2, buffers);
		systems.pD3DContext->PSSetConstantBuffers(0, 2, buffers);

		// Bind a sampler state
		ID3D11SamplerState* samplers[] = { m_pLinearMipSamplerState };
		systems.pD3DContext->PSSetSamplers(0, 1, samplers);


		constexpr f32 kGridSpacing = 1.5f;
		constexpr u32 kNumInstances = 5;
		constexpr u32 kNumModelTypes = 2;

		for(u32 i = 0; i < kNumModelTypes; ++i)
		{
			// Bind a mesh and texture.
			m_meshArray[i].bind(systems.pD3DContext);
			m_textures[0].bind(systems.pD3DContext, ShaderStage::kPixel, 0);
			m_textures[1].bind(systems.pD3DContext, ShaderStage::kPixel, 1);

			// Draw several instances
			for (u32 j = 0; j < kNumInstances; ++j)
			{
				// Compute MVP matrix.
				m4x4 matWorld = m4x4::CreateTranslation(v3(j * kGridSpacing, i * kGridSpacing, 0.f));
				m4x4 matMVP = matWorld * systems.pCamera->vpMatrix;


				// Update Per Draw Data
				m_perDrawCBData.m_matMVP = matMVP.Transpose();
				m_perDrawCBData.m_matWorld = matWorld.Transpose();
				// Inverse transpose,  but since we didn't do any shearing or non-uniform scaling then we simple grab the upper 3x3 in the shader.
				pack_upper_float3x3(m_perDrawCBData.m_matWorld, m_perDrawCBData.m_matNormal);

				// Push to GPU
				push_constant_buffer(systems.pD3DContext, m_pPerDrawCB, m_perDrawCBData);

				// Draw the mesh.
				m_meshArray[i].draw(systems.pD3DContext);
			}
		}

	}



	void on_resize(SystemsInterface& ) override
	{
		
	}

private:

	PerFrameCBData m_perFrameCBData;
	ID3D11Buffer* m_pPerFrameCB = nullptr;

	PerDrawCBData m_perDrawCBData;
	ID3D11Buffer* m_pPerDrawCB = nullptr;

	ShaderSet m_meshShader;
	
	Mesh m_meshArray[2];
	Texture m_textures[2];
	ID3D11SamplerState* m_pLinearMipSamplerState = nullptr;

	v3 m_position;
	f32 m_size;
};

NormalMappingApp g_app;

FRAMEWORK_IMPLEMENT_MAIN(g_app, "Normal Maps")
