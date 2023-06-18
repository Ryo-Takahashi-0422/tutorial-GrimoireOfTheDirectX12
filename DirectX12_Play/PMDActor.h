#pragma once

class PMDActor
{
private:
	DWORD _startTime; // �A�j���[�V�����J�n���̃~���b
	DWORD elapsedTime; // �o�߃~���b
	unsigned int frameNo; // ���݂̃t���[��No

public:
	unsigned int GetFrameNo();
	void PlayAnimation();
	void MotionUpdate(unsigned int maxFrameNum);
	float GetYFromXOnBezier(float x,const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n);
};