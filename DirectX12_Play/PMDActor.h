#pragma once

class PMDActor
{
private:
	DWORD _startTime; // アニメーション開始時のミリ秒
	DWORD elapsedTime; // 経過ミリ秒
	unsigned int frameNo; // 現在のフレームNo

public:
	unsigned int GetFrameNo();
	void PlayAnimation();
	void MotionUpdate(unsigned int maxFrameNum);
	float GetYFromXOnBezier(float x,const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n);
};