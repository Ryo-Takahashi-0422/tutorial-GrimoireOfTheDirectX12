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
	elapsedTime = timeGetTime() - _startTime; // �o�ߎ��Ԃ𑪒肵�Ċi�[
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
		return x; // �v�Z�s�v
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x; // t^3�W��
	const float k1 = 3 * b.x - 6 * a.x; // t^2�W��
	const float k2 = 3 * a.x; // t�W��

	// �덷�͈͓̔����ǂ������肷�邽�߂̒萔
	constexpr float epsilon = 0.0005f;

	// t���ߎ��ŋ��߂�
	for (int i = 0; i < n; ++i) 
	{
		// f(t)�����߂�
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;

		// �������ʂ�0�ɋ߂�(�덷�͈͓̔�)�Ȃ�ł��؂�
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}

		t -= ft / 2; // ����������
	}

	// ���߂���t�͊��ɋ��߂Ă���̂�y���v�Z����
	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.x + 3 * t * r * r * a.x;
}