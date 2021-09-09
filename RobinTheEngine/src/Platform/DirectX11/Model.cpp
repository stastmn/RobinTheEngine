#include "rtepch.h"
#include "Model.h"
#include "RobinTheEngine/d3dUtils.h"
#include "Platform/DirectX11/DirectX11RenderSystem.h"
#include "RobinTheEngine/Application.h"

using namespace D3DUtils;

bool RTE::Model::Initialize(const std::string& path, ConstantBuffer<CB_VS_MATRIX4x4>& cb_vs_vertexshader)
{
	DirectX11RenderSystem* rs = static_cast<DirectX11RenderSystem*> (Application::Get().GetRenderSystem());

	this->device = rs->GetDevice().Get();
	this->deviceContext = rs->GetContext().Get();
	this->cb_vs_vertexshader = &cb_vs_vertexshader;
	GetDirectoryFromPath(path);

	if (!this->LoadModel(path)) {
		std::string warn = "Cant load model with path: " + path;
		RTE_CORE_WARN(warn);
		return false;
	}

	return true;
}

void RTE::Model::SetTexture(ID3D11ShaderResourceView * texture)
{
	this->texture = texture;
}

void RTE::Model::Draw(const XMMATRIX& worldMatrix, const XMMATRIX & viewProjectionMatrix)
{
	//Update Constant buffer with WVP Matrix
	XMStoreFloat4x4(&cb_vs_vertexshader->data.mvpMatrix, XMMatrixTranspose(worldMatrix * viewProjectionMatrix)); //Calculate World-View-Projection Matrix
	XMStoreFloat4x4(&cb_vs_vertexshader->data.worldMatrix, XMMatrixTranspose(worldMatrix)); 

	cb_vs_vertexshader->WriteBuffer();
	this->deviceContext->VSSetConstantBuffers(0, 1, this->cb_vs_vertexshader->GetAddressOf());

	// draw sub meshes
	for (int i = 0; i < meshes.size(); i++)
	{
		meshes[i].Draw();
	}
}


bool RTE::Model::LoadModel(const std::string & filePath)
{
	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(filePath,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded);

	if (pScene == NULL)
		return false;


	this->ProcessNode(pScene->mRootNode, pScene);
	return true;
}

void RTE::Model::ProcessNode(aiNode * node, const aiScene * scene)
{
	for (UINT i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(this->ProcessMesh(mesh, scene));
	}

	for (UINT i = 0; i < node->mNumChildren; i++)
	{
		this->ProcessNode(node->mChildren[i], scene);
	}
}

RTE::Mesh RTE::Model::ProcessMesh(aiMesh * mesh, const aiScene * scene)
{
	// Data to fill
	std::vector<vertex_Gouraud_shading> vertices;
	std::vector<DWORD> indices;

	//Get vertices
	for (UINT i = 0; i < mesh->mNumVertices; i++)
	{
		vertex_Gouraud_shading vertex;

		vertex.pos.x = mesh->mVertices[i].x;
		vertex.pos.y = mesh->mVertices[i].y;
		vertex.pos.z = mesh->mVertices[i].z;

		vertex.normal.x = mesh->mNormals[i].x;
		vertex.normal.y = mesh->mNormals[i].y;
		vertex.normal.z = mesh->mNormals[i].z;

		if (mesh->mTextureCoords[0])
		{
			vertex.texCoord.x = (float)mesh->mTextureCoords[0][i].x;
			vertex.texCoord.y = (float)mesh->mTextureCoords[0][i].y;
		}

		//if ( mesh->mNormals[0] ) {
		//	vertex.normalCoord.x = (float)mesh->mNormals[i].x;
		//	vertex.normalCoord.y = (float)mesh->mNormals[i].y;

		//}

		vertices.push_back(vertex);
	}

	//Get indices
	for (UINT i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		for (UINT j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	assert(mesh->mMaterialIndex >= 0);
	std::vector<Texture> textures;
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	std::vector<Texture> diffuseTextures = LoadMaterialTextures(material, aiTextureType::aiTextureType_DIFFUSE, scene);
	textures.insert(textures.end(), diffuseTextures.begin(), diffuseTextures.end());

	return Mesh(vertices, indices, textures);

}

void RTE::Model::GetDirectoryFromPath(const std::string & path)
{
	size_t off1 = path.find_last_of('\\');
	size_t off2 = path.find_last_of('/');
	if (off1 == std::string::npos && off2 == std::string::npos) //If no slash or backslash in path?
	{
		this->directory = "";
		return;
	}
	if (off1 == std::string::npos)
	{
		this->directory = path.substr(0, off2 - 1);
		return;
	}
	if (off2 == std::string::npos)
	{
		this->directory = path.substr(0, off1 - 1);
		return;
	}
	//If both exists, need to use the greater offset
	this->directory = path.substr(0, std::max(off1, off2) - 1);
}


int RTE::Model::GetTextureIndex(aiString * pStr)
{
	assert(pStr->length >= 2);
	return atoi(&pStr->C_Str()[1]);
}


std::vector<RTE::Texture> RTE::Model::LoadMaterialTextures(aiMaterial* pMaterial, aiTextureType textureType, const aiScene* pScene)
{
	std::vector<Texture> materialTextures;
	unsigned int textureCount = pMaterial->GetTextureCount(textureType);

	if (textureCount == 0) //If there are no textures
	{
		switch (textureType)
		{
		case aiTextureType_DIFFUSE:
			materialTextures.push_back(RTE::Texture("Bad path", textureType));
			return materialTextures;
		}
	}
	else //If there are textures, 
	{
		for (UINT i = 0; i < textureCount; i++)
		{
			aiString path;
			pMaterial->GetTexture(textureType, i, &path);

			std::string filename = this->directory + '\\' + path.C_Str();
			Texture diskTexture(filename, textureType);
			materialTextures.push_back(diskTexture);
			break;

		}
	}


	assert(materialTextures.size() != 0);

	return materialTextures;
}
