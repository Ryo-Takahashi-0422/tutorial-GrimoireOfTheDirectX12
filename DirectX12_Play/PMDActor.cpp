#include <stdafx.h>
#pragma comment(lib, "winmm.lib")

unsigned int PMDActor::GetFrameNo() 
{
	return frameNo;
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