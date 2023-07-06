#pragma once

class TextureLoader
{
private:
	std::map<std::string, LoadLambda_t> loadLambdaTable;
	void SetTable();

public:
	//�t�@�C���`�����̃e�N�X�`�����[�h����
	void LoadTexture();
	std::map<std::string, LoadLambda_t> GetTable();
};