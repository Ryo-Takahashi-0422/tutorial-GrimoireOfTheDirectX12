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
	elapsedTime = timeGetTime() - _startTime; // Œo‰ßŠÔ‚ğ‘ª’è‚µ‚ÄŠi”[
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
		return x; // ŒvZ•s—v
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x; // t^3ŒW”
	const float k1 = 3 * b.x - 6 * a.x; // t^2ŒW”
	const float k2 = 3 * a.x; // tŒW”

	// Œë·‚Ì”ÍˆÍ“à‚©‚Ç‚¤‚©”»’è‚·‚é‚½‚ß‚Ì’è”
	constexpr float epsilon = 0.0005f;

	// t‚ğ‹ß—‚Å‹‚ß‚é
	for (int i = 0; i < n; ++i) 
	{
		// f(t)‚ğ‹‚ß‚é
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;

		// ‚à‚µŒ‹‰Ê‚ª0‚É‹ß‚¢(Œë·‚Ì”ÍˆÍ“à)‚È‚ç‘Å‚¿Ø‚é
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}

		t -= ft / 2; // ”¼Œ¸‚³‚¹‚é
	}

	// ‹‚ß‚½‚¢t‚ÍŠù‚É‹‚ß‚Ä‚¢‚é‚Ì‚Åy‚ğŒvZ‚·‚é
	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.x + 3 * t * r * r * a.x;
}