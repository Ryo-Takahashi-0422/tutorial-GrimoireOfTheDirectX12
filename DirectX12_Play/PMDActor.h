#pragma once

class PMDActor
{
private:
	DWORD _startTime; // アニメーション開始時のミリ秒
	DWORD elapsedTime; // 経過ミリ秒
	unsigned int frameNo; // 現在のフレームNo
	std::vector<PMDIK> _ikData; // ikデータ群
	PMDMaterialInfo* pmdMaterialInfo;
	std::vector<DirectX::XMMATRIX> boneMatrices;

public:
	// コピーコンストラクタ
	PMDActor(PMDMaterialInfo* _pmdMatInfo);
	unsigned int GetFrameNo();
	void PlayAnimation();
	void MotionUpdate(unsigned int maxFrameNum);
	float GetYFromXOnBezier(float x,const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n);

	// z軸を特定の方向に向かせる行列を返す関数
	// @param lookat 向かせたい方向ベクトル
	// @param up 上ベクトル
	// @param right 右ベクトル
	XMMATRIX LookAtMatrix(const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right);

	///特定のベクトルを特定の方向に向けるための行列を返す
	///@param origin 特定のベクトル
	///@param lookat 向かせたい方向
	///@param up 上ベクトル
	///@param right 右ベクトル
	///@retval 特定のベクトルを特定の方向に向けるための行列
	XMMATRIX LookAtMatrix(const XMVECTOR& origin, const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right);
	
	// IKの導出パターンを決定する
	void IKSolve();

	// CCD-IKによりボーン方向を決定 node構成：root-interminiate(2個以上)-target
	// @param ik 対象IKオブジェクト
	void SolveCCDIK(const PMDIK& ik);

	// 余弦定理IKによりボーン方向を決定 node構成：root-interminiate-target
	// @param ik 対象IKオブジェクト
	void SolveCosineIK(const PMDIK& ik);

	// LOOKAT行列によりボーン方向を決定 node構成：root-target
	// @param ik 対象IKオブジェクト
	void SolveLookAtIK(const PMDIK& ik);
};