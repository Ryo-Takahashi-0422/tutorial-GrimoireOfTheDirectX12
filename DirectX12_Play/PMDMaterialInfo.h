#pragma once
using namespace DirectX;

	struct Vertex //?　下のvertices直下に宣言すると、std::copyでエラー発生する
	{
		XMFLOAT3 pos; // xyz座標
		XMFLOAT2 uv; // uv座標
	};

	//PMDヘッダー構造体
	struct PMDHeader
	{
		float version; // 00 00 80 3F == 1.00
		char model_name[20]; // モデル名
		char comment[256]; // コメント
	};

	//PMD 頂点構造体
	struct PMDVertex
	{   //38byte
		float pos[3]; // x, y, z // 座標 12byte
		float normal_vec[3]; // nx, ny, nz // 法線ベクトル 12byte
		float uv[2]; // u, v // UV座標 // MMDは頂点UV 8byte
		unsigned short bone_num[2]; // ボーン番号1、番号2 // モデル変形(頂点移動)時に影響 4byte
		unsigned char bone_weight; // ボーン1に与える影響度 // min:0 max:100 // ボーン2への影響度は、(100 - bone_weight) 1byte
		unsigned char edge_flag; // 0:通常、1:エッジ無効 // エッジ(輪郭)が有効の場合 1byte
	};

	//シェーダー側に渡す基本的な行列データ
	struct SceneMatrix
	{
		XMMATRIX world; // モデル本体の回転・移動行列
		XMMATRIX view; // ビュー行列
		XMMATRIX proj; // プロジェクション行列
		XMFLOAT3 eye; // 視点座標
		XMMATRIX bones[256]; // ボーン行列
	};

	//マテリアル読み込み用の構造体2セット
	struct PMDMaterialSet1
	{ //46Byte読み込み用
		XMFLOAT3 diffuse;
		float alpha;
		float specularity;
		XMFLOAT3 specular;
		XMFLOAT3 ambient;
		unsigned char toonIdx;
		unsigned char edgeFlg;
	};

	struct PMDMaterialSet2
	{ //24Byte読み込み用
		unsigned int indicesNum;
		char texFilePath[20];
	};

	//読み込んだマテリアル構造体の出力用に分類
	struct MaterialForHlsl // メイン
	{
		XMFLOAT3 diffuse;
		float alpha;
		XMFLOAT3 specular;
		float specularity;
		XMFLOAT3 ambient;
	};

	struct AdditionalMaterial // サブ
	{
		std::string texPath;
		int toonIdx;
		bool edgeFlg;
	};

	struct Material // 集合体
	{
		unsigned int indiceNum;
		MaterialForHlsl material;
		AdditionalMaterial addtional;
	};

#pragma pack(1)
	struct PMDBone // ボーン情報
	{
		char boneName[20]; // ボーン名
		unsigned short parentIndex; // 親ボーン番号(ない場合は0xFFFF)
		unsigned short tailIndex; // tail位置のボーン番号(チェーン末端の場合は0xFFFF 0 →補足2) // 親：子は1：多なので、主に位置決め用
		unsigned char type; // ボーンの種類
		unsigned short ikParentIndex; // IKボーン番号(影響IKボーン。ない場合は0)
		XMFLOAT3 headPos; // ボーンのヘッドの位置
	};
#pragma pack()

	struct BoneNode
	{
		uint32_t boneIdx; // ボーンインデックス
		uint32_t boneType; // ボーン種別 0:回転,1:回転＆移動,2:IK,...7:不可視ボーン
		uint32_t ikParentBone; // IK親ボーン
		XMFLOAT3 startPos; // ボーン基準点(回転中心)
		//XMFLOAT3 endPos; // ボーン先端点
		std::vector<BoneNode*> children; // 子ノード
	};

	struct PMDIK // PMDモデル内IK情報
	{
		uint16_t boneidx; // IKボーン番号
		uint16_t targetidx; // IKターゲットボーン番号 // IKボーンが最初に接続するボーン		
		uint16_t iterations; // 再帰演算回数 // IK値1
		float limit; // 演算1回あたりの制限角度 // IK値2
		std::vector<uint16_t> nodeIdx; // IK影響下のボーン番号
	};

class PMDMaterialInfo
{
private:
	unsigned short boneNum;
	std::vector<DirectX::XMMATRIX> _boneMatrice;
	std::vector<PMDBone> pmdBones;
	std::map<std::string, BoneNode> _boneNodeTable;
	std::vector<std::string> boneName;
	uint16_t ikNum;
	uint8_t chainLen; // IKチェーンの長さ(間のノード数)
	std::vector<PMDIK> pmdIkData; // IK情報コンテナ

public:

	HRESULT ReadPMDHeaderFile(std::string);

	float angle;
	XMMATRIX worldMat;
	SceneMatrix* mapMatrix = nullptr;

	//std::string strModelPath = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\初音ミク.pmd";
	char signature[3] = {}; // シグネチャ
	PMDHeader pmdHeader = {};
	unsigned int vertNum; // 頂点数
	size_t pmdvertex_size;
	std::vector<unsigned char> vertices{};
	std::vector<unsigned short> indices{};
	unsigned int indicesNum;
	unsigned int materialNum;
	std::vector<PMDMaterialSet1> pmdMat1;
	std::vector<PMDMaterialSet2> pmdMat2;
	std::vector<Material> materials;
	//std::vector<DirectX::XMMATRIX> GetBoneMatrices();
	size_t GetNumberOfBones();
	std::map<std::string, BoneNode> GetBoneNode();

	std::vector<std::string> _boneNameArray; // インデックスからボーン名を検索する用
	std::vector<BoneNode*> _boneNodeAddressArray; // インデックスからノードを検索する用
};
