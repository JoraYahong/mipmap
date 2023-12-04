#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "utils.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <unordered_map>
#include "glad.h"
#include <cstdlib>

#include "stb_image.h"
// vertex of an animated model
struct Vertex {
	glm::vec3 position;							//顶点
	glm::vec3 normal;							//法向
	glm::vec2 uv;								//纹理坐标
	glm::vec4 boneIds = glm::vec4(0);			//骨骼 
	glm::vec4 boneWeights = glm::vec4(0.0f);	//骨骼的权重
};

// structure to hold bone tree (skeleton) 骨骼树
struct Bone {
	int id = 0; // position of the bone in final upload array
	std::string name = "";										//名称
	glm::mat4 offset = glm::mat4(1.0f);
	std::vector<Bone> children = {};							//子结点集合
};

// sturction representing an animation track 动画描述
struct BoneTransformTrack {
	std::vector<float> positionTimestamps = {};					//移动所费的时间 
	std::vector<float> rotationTimestamps = {};
	std::vector<float> scaleTimestamps = {};

	std::vector<glm::vec3> positions = {};						//移动
	std::vector<glm::quat> rotations = {};
	std::vector<glm::vec3> scales = {};
};

// structure containing animation information 动画
struct Animation {
	float duration = 0.0f;														//动画时间
	float ticksPerSecond = 1.0f;												//时间单位
	std::unordered_map<std::string, BoneTransformTrack> boneTransforms = {};	//动画序列map
};



// a recursive function to read all bones and form skeleton
bool readSkeleton(Bone& boneOutput, aiNode* node, std::unordered_map<std::string, std::pair<int, glm::mat4>>& boneInfoTable) {

	if (boneInfoTable.find(node->mName.C_Str()) != boneInfoTable.end()) { // if node is actually a bone
		boneOutput.name = node->mName.C_Str();
		boneOutput.id = boneInfoTable[boneOutput.name].first;
		boneOutput.offset = boneInfoTable[boneOutput.name].second;

		//打印骨骼信息
		std::cout << "readSkeleton() bone=" << boneOutput.name << std::endl;

		for (int i = 0; i < node->mNumChildren; i++) {
			Bone child;
			readSkeleton(child, node->mChildren[i], boneInfoTable);
			boneOutput.children.push_back(child);
		}
		return true;
	}
	else { // find bones in children
		for (int i = 0; i < node->mNumChildren; i++) {
			if (readSkeleton(boneOutput, node->mChildren[i], boneInfoTable)) {
				return true;
			}

		}
	}
	return false;
}

void loadModel(const aiScene* scene, aiMesh* mesh, std::vector<Vertex>& verticesOutput, std::vector<uint>& indicesOutput, Bone& skeletonOutput, uint& nBoneCount) {

	verticesOutput = {};
	indicesOutput = {};
	//load position, normal, uv
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		//process position 
		Vertex vertex;
		glm::vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.position = vector;
		//process normal
		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.normal = vector;
		//process uv
		glm::vec2 vec;
		vec.x = mesh->mTextureCoords[0][i].x;
		vec.y = mesh->mTextureCoords[0][i].y;
		vertex.uv = vec;

		vertex.boneIds = glm::ivec4(0);
		vertex.boneWeights = glm::vec4(0.0f);

		verticesOutput.push_back(vertex);
	}

	//load boneData to vertices
	std::unordered_map<std::string, std::pair<int, glm::mat4>> boneInfo = {};
	std::vector<uint> boneCounts;
	boneCounts.resize(verticesOutput.size(), 0);
	nBoneCount = mesh->mNumBones;
	//打印骨骼总数
	std::cout << "loadModel() nBoneCount=" << nBoneCount << "\n" << std::endl;
	//loop through each bone
	for (uint i = 0; i < nBoneCount; i++) {
		aiBone* bone = mesh->mBones[i];
		glm::mat4 m = assimpToGlmMatrix(bone->mOffsetMatrix);
		boneInfo[bone->mName.C_Str()] = { i, m };

		//loop through each vertex that have that bone
		for (int j = 0; j < bone->mNumWeights; j++) {
			uint id = bone->mWeights[j].mVertexId;
			float weight = bone->mWeights[j].mWeight;
			boneCounts[id]++;
			switch (boneCounts[id]) {
			case 1:
				verticesOutput[id].boneIds.x = i;
				verticesOutput[id].boneWeights.x = weight;
				break;
			case 2:
				verticesOutput[id].boneIds.y = i;
				verticesOutput[id].boneWeights.y = weight;
				break;
			case 3:
				verticesOutput[id].boneIds.z = i;
				verticesOutput[id].boneWeights.z = weight;
				break;
			case 4:
				verticesOutput[id].boneIds.w = i;
				verticesOutput[id].boneWeights.w = weight;
				break;
			default:
				//std::cout << "err: unable to allocate bone to vertex" << std::endl;
				break;

			}
		}
	}

	//normalize weights to make all weights sum 1
	for (int i = 0; i < verticesOutput.size(); i++) {
		glm::vec4& boneWeights = verticesOutput[i].boneWeights;
		float totalWeight = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
		if (totalWeight > 0.0f) {
			verticesOutput[i].boneWeights = glm::vec4(
				boneWeights.x / totalWeight,
				boneWeights.y / totalWeight,
				boneWeights.z / totalWeight,
				boneWeights.w / totalWeight
			);
		}
	}


	//load indices
	for (int i = 0; i < mesh->mNumFaces; i++) {
		aiFace& face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indicesOutput.push_back(face.mIndices[j]);
	}

	// create bone hirerchy
	readSkeleton(skeletonOutput, scene->mRootNode, boneInfo);
}

void loadAnimation(const aiScene* scene, Animation& animation) {
	//loading  first Animation
	aiAnimation* anim = scene->mAnimations[0];

	if (anim->mTicksPerSecond != 0.0f)
		animation.ticksPerSecond = anim->mTicksPerSecond;
	else
		animation.ticksPerSecond = 1;


	animation.duration = anim->mDuration * anim->mTicksPerSecond;
	animation.boneTransforms = {};

	//duration 动画时间间隔  ticksPerSecond 时间单位
	std::cout << "loadAnimation() ticksPerSecond=" << animation.ticksPerSecond << " duration=" << animation.duration << "\n" << std::endl;

	//现在来修复这个bug
	bool checkAssimpFbx = false;						//是否是AssimpFbx动画
	std::string assimpFbxStr;							//存储AssimpFbx动画的名称
	std::vector<BoneTransformTrack> assimpFbxVector;	//存储AssimpFbx的动画序列
	//load positions rotations and scales for each bone
	// each channel represents each bone
	for (int i = 0; i < anim->mNumChannels; i++) {
		aiNodeAnim* channel = anim->mChannels[i];
		BoneTransformTrack track;
		for (int j = 0; j < channel->mNumPositionKeys; j++) {
			track.positionTimestamps.push_back(channel->mPositionKeys[j].mTime);
			track.positions.push_back(assimpToGlmVec3(channel->mPositionKeys[j].mValue));
		}
		for (int j = 0; j < channel->mNumRotationKeys; j++) {
			track.rotationTimestamps.push_back(channel->mRotationKeys[j].mTime);
			track.rotations.push_back(assimpToGlmQuat(channel->mRotationKeys[j].mValue));

		}
		for (int j = 0; j < channel->mNumScalingKeys; j++) {
			track.scaleTimestamps.push_back(channel->mScalingKeys[j].mTime);
			track.scales.push_back(assimpToGlmVec3(channel->mScalingKeys[j].mValue));

		}

		std::string nName(channel->mNodeName.C_Str());
		std::string::size_type ret = nName.find("_$AssimpFbx$_");
		//如果是FBX动画则先暂存
		if (ret != std::string::npos)
		{
			checkAssimpFbx = true;
			assimpFbxStr = nName.substr(0, ret);
			assimpFbxVector.push_back(track);
			std::cout << "loadAnimation() print assimpFbxStr=" << assimpFbxStr << " OldStr=" << nName << std::endl;
		}
		else {
			//如果是FBX动画刚结束，则暂存动画保存至animation
			if (checkAssimpFbx)
			{
				checkAssimpFbx = false;
				//用于暂存的变量
				BoneTransformTrack outTrack;
				for (int i = 0; i < assimpFbxVector.size(); i++)
				{
					BoneTransformTrack item = assimpFbxVector[i];
					if (item.positions.size() > 1)
					{
						outTrack.positionTimestamps = item.positionTimestamps;
						outTrack.positions = item.positions;
					}
					if (item.rotations.size() > 1)
					{
						outTrack.rotationTimestamps = item.rotationTimestamps;
						outTrack.rotations = item.rotations;
					}
					if (item.scales.size() > 1)
					{
						outTrack.scaleTimestamps = item.scaleTimestamps;
						outTrack.scales = item.scales;
					}
				}
				std::cout << "loadAnimation() animation FBX=" << assimpFbxStr << std::endl;
				animation.boneTransforms[assimpFbxStr] = outTrack;

			}
			//打印动画信息 骨骼(bone)与动画(animation)信息是一对一关系!
			std::cout << "loadAnimation() animation=" << channel->mNodeName.C_Str() << std::endl;
			animation.boneTransforms[channel->mNodeName.C_Str()] = track;
		}

	}
}

unsigned int createVertexArray(std::vector<Vertex>& vertices, std::vector<uint> indices) {
	uint
		vao = 0,
		vbo = 0,
		ebo = 0;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, boneIds));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, boneWeights));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint), &indices[0], GL_STATIC_DRAW);
	glBindVertexArray(0);
	return vao;
}

uint createTexture(std::string filepath) {
	uint textureId = 0;
	int width, height, nrChannels;
	byte* data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 4);
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);
	return textureId;
}



std::pair<uint, float> getTimeFraction(std::vector<float>& times, float& dt) {
	uint segment = 0;//times是个空数组 故报错	
	//dt = 276.320007 但times[1]=276 segment百分百的越界
	while (dt > times[segment])
	{
		segment++;
		if (segment >= times.size()) {
			segment--;
			break;
		}
	}

	float start = times[segment - 1];
	float end = times[segment];
	float frac = (dt - start) / (end - start);
	return { segment, frac };
}



void getPose(Animation& animation, Bone& skeletion, float dt, std::vector<glm::mat4>& output, glm::mat4& parentTransform, glm::mat4& globalInverseTransform) {
	BoneTransformTrack& btt = animation.boneTransforms[skeletion.name];//根据骨骼名"mixamorig:Hips" btt没有找到动画信息! 这里是出事地点，却不是案发现场
	//因为骨骼(bone)与动画(animation)信息是一对一关系
	//所以问题就很明显: 
	//在控制台里 骨骼mixamorig:Hips 对应的动画信息是 mixamorig:Hips_$AssimpFbx$_Translation Hips_$AssimpFbx$_Rotation Hips_$AssimpFbx$_Scaling三个动画
	//所以需要将刚才三个动画合成一个动画，并且名称改成 mixamorig:Hips

	//再一次的vector溢出表示仍然存在着更深的bug
	//如果老实的一句一句的跟断点将永远找不着出事地点
	//从控制台里可以看到bone=空

	//但愿这是最后一次bug。 非常确定这个bug藏得更深。
	//从控制台可以看到 打印bone到 mixamorig:LeftHand 卡住。但却有无数个循环。。。接着看吧
	//图像编程里最常见的BUG就是循环里或者嵌套循环里有BUG。。。
	//利用时间刻度来打印看看吧。
	//利用时间刻度和骨骼名两重断点的判断就能找到出事地点
	if (btt.positions.size() == 0 || btt.rotations.size() == 0 || btt.scales.size() == 0)
		return;

	//if(skeletion.name == "mixamorig:LeftHand" )
		//std::cout << "getPose() bone=" << skeletion.name << std::endl;

	dt = fmod(dt, animation.duration);
	std::pair<uint, float> fp;
	//calculate interpolated position
	fp = getTimeFraction(btt.positionTimestamps, dt);

	glm::vec3 position1 = btt.positions[fp.first - 1];
	glm::vec3 position2 = btt.positions[fp.first];

	glm::vec3 position = glm::mix(position1, position2, fp.second);

	//calculate interpolated rotation
	fp = getTimeFraction(btt.rotationTimestamps, dt);
	glm::quat rotation1 = btt.rotations[fp.first - 1];
	glm::quat rotation2 = btt.rotations[fp.first];

	glm::quat rotation = glm::slerp(rotation1, rotation2, fp.second);

	//calculate interpolated scale
	fp = getTimeFraction(btt.scaleTimestamps, dt);
	glm::vec3 scale1 = btt.scales[fp.first - 1];
	glm::vec3 scale2 = btt.scales[fp.first];

	glm::vec3 scale = glm::mix(scale1, scale2, fp.second);

	glm::mat4 positionMat = glm::mat4(1.0),
		scaleMat = glm::mat4(1.0);


	// calculate localTransform
	positionMat = glm::translate(positionMat, position);
	glm::mat4 rotationMat = glm::toMat4(rotation);
	scaleMat = glm::scale(scaleMat, scale);
	glm::mat4 localTransform = positionMat * rotationMat * scaleMat;
	glm::mat4 globalTransform = parentTransform * localTransform;

	output[skeletion.id] = globalInverseTransform * globalTransform * skeletion.offset;
	//update values for children bones
	for (Bone& child : skeletion.children) {
		getPose(animation, child, dt, output, globalTransform, globalInverseTransform);
	}
	//std::cout << dt << " => " << position.x << ":" << position.y << ":" << position.z << ":" << std::endl;
}

