#include <stdafx.h>
#pragma comment(lib, "winmm.lib")

namespace
{
	enum class BoneType
	{
		Rotation,		// 回転
		RotAndMove,		// 回転＆移動
		IK,				// IK
		Undefine,		// 未定義
		IKChild,		// IK影響ボーン
		RotationChild,	// 回転影響ボーン
		IKDestination,	// IK接続先
		Invisible		// 不可視ボーン
	};
}

PMDActor::PMDActor(PMDMaterialInfo* _pmdMatInfo) : pmdMaterialInfo(_pmdMatInfo)
{
	boneMatrices = pmdMaterialInfo->GetBoneMatrices();

}

XMMATRIX PMDActor::LookAtMatrix(const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right)
{
	// 向かせたい方向(Z軸ベクトル)
	XMVECTOR vz = lookat;

	// (向かせたい方向を向かせたときの)仮Y軸
	XMVECTOR vy = XMVector3Normalize(XMLoadFloat3(&up));

	// 向かせたい方向を向かせたときのX軸を計算
	XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));

	// 真のY軸を計算
	vy = XMVector3Cross(vz, vx);

	// もしLookAtとupが同じ方向を向いていたらrightを基準にして作り直す
	if (std::abs(XMVector3Dot(vy, vz).m128_f32[0]) == 1.0f)
	{
		// (向かせたい方向を向かせたときの)仮Xベクトル
		vx = XMVector3Normalize(XMLoadFloat3(&right));

		// 向かせたい方向を向かせたときのY軸を計算
		vy = XMVector3Normalize(XMVector3Cross(vz, vx));

		//真のX軸を計算
		vx = XMVector3Normalize(XMVector3Cross(vy, vz));
	}

	XMMATRIX ret = XMMatrixIdentity();
	ret.r[0] = vx;
	ret.r[1] = vy;
	ret.r[2] = vx;
	return ret;
}

XMMATRIX PMDActor::LookAtMatrix(const XMVECTOR& origin, const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right) {
	return XMMatrixTranspose(LookAtMatrix(origin, up, right)) *	LookAtMatrix(lookat, up, right);
}

void PMDActor::IKSolve()
{
	for (auto& ik : _ikData)
	{
		auto childrenNodeCount = ik.nodeIdx.size();
		switch (childrenNodeCount)
		{
		case 0: // 間のボーンが0(ケース無し)
			continue;
		case 1: // 間のボーンが1の時はLookAt
			SolveLookAtIK(ik);
		case 2: // 間のボーンが2の時は余弦定理
			SolveCosineIK(ik);
		default: // 間のボーンが3の時はCCD-IK
			SolveCCDIK(ik);
		}
	}
}

void PMDActor::SolveCCDIK(const PMDIK& ik)
{

}

void PMDActor::SolveCosineIK(const PMDIK& ik)
{

}

void PMDActor::SolveLookAtIK(const PMDIK& ik) 
{
	// root→targetへ向かうベクトルを導出
	auto rootNode = pmdMaterialInfo->_boneNodeAddressArray[ik.nodeIdx[0]];
	auto targetNode = pmdMaterialInfo->_boneNodeAddressArray[ik.targetidx];

	auto rpos1 = XMLoadFloat3(&rootNode->startPos);
	auto tpos1 = XMLoadFloat3(&targetNode->startPos);

	auto rpos2 = XMVector3TransformCoord(rpos1, boneMatrices[ik.nodeIdx[0]]);
}

void PMDActor::PlayAnimation() 
{
	_startTime = timeGetTime();
}

void PMDActor::MotionUpdate(unsigned int maxFrameNum)
{
	elapsedTime = timeGetTime() - _startTime; // 経過時間を測定して格納
	frameNo = 30 * (elapsedTime / 1000.0f);

	if (frameNo > maxFrameNum)
	{
		PlayAnimation();
		frameNo = 0;
	}
}

unsigned int PMDActor::GetFrameNo()
{
	return frameNo;
}

float PMDActor::GetYFromXOnBezier(float x, const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n)
{
	if (a.x == a.y && b.x == b.y)
	{
		return x; // 計算不要
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x; // t^3係数
	const float k1 = 3 * b.x - 6 * a.x; // t^2係数
	const float k2 = 3 * a.x; // t係数

	// 誤差の範囲内かどうか判定するための定数
	constexpr float epsilon = 0.0005f;

	// tを近似で求める
	for (int i = 0; i < n; ++i) 
	{
		// f(t)を求める
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;

		// もし結果が0に近い(誤差の範囲内)なら打ち切る
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}

		t -= ft / 2; // 半減させる
	}

	// 求めたいtは既に求めているのでyを計算する
	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.x + 3 * t * r * r * a.x;
}