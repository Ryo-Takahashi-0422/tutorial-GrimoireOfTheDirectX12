#include <stdafx.h>

LRESULT PrepareRenderingWindow::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_DESTROY:
		MessageBox(hwnd, TEXT("quit application"),
			TEXT("quit"), MB_ICONINFORMATION);
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wp, lp);
}

// window�����b�Z�[�W���[�v���Ɏ擾�������b�Z�[�W����������N���X
LRESULT CALLBACK PrepareRenderingWindow::StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{	
	PrepareRenderingWindow* This = (PrepareRenderingWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (!This) {//�擾�ł��Ȃ�����(�E�B���h�E������)�ꍇ
		if (msg == WM_CREATE || msg == WM_INITDIALOG) {
			This = (PrepareRenderingWindow*)((LPCREATESTRUCT)lparam)->lpCreateParams;
			if (This) {				
				SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)This);
				return This->WndProc(hwnd, msg, wparam, lparam);
			}
		}
	}

	else {//�擾�ł����ꍇ(�E�B���h�E������)
		return This->WndProc(hwnd, msg, wparam, lparam);
	}

	switch (msg)
	{
	case WM_DESTROY: // process when the window is closed
		MessageBox(hwnd, TEXT("Thanks for starting"),
			TEXT("quit the application"), MB_ICONINFORMATION);
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void PrepareRenderingWindow::CreateAppWindow()
{
	// �E�B���h�E�N���X�̐����Ə�����
	w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)StaticWndProc;
	w.lpszClassName = _T("DX12Sample");
	w.hInstance = GetModuleHandle(nullptr);

	// ��L�E�B���h�E�N���X�̓o�^�BWINDCLASSEX�Ƃ��Ĉ�����B
	RegisterClassEx(&w);

	RECT wrc = { 0,0,window_width, window_height };

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	hwnd = CreateWindow(
		w.lpszClassName,
		_T("DX12test"),//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,//�^�C�g���o�[�Ƌ��E��������E�B���h�E�ł�
		CW_USEDEFAULT,//�\��X���W��OS�ɂ��C�����܂�
		CW_USEDEFAULT,//�\��Y���W��OS�ɂ��C�����܂�
		wrc.right - wrc.left,//�E�B���h�E��
		wrc.bottom - wrc.top,//�E�B���h�E��
		nullptr,//�e�E�B���h�E�n���h��
		nullptr,//���j���[�n���h��
		w.hInstance,//�Ăяo���A�v���P�[�V�����n���h��
		nullptr);//�ǉ��p�����[�^
}

void PrepareRenderingWindow::SetViewportAndRect()
{
	viewport = {};
	viewport.Width = window_width;
	viewport.Height = window_height;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;

	scissorRect = {};
	scissorRect.top = 0; //�؂蔲������W
	scissorRect.left = 0; //�؂蔲�������W
	scissorRect.right = scissorRect.left + window_width; //�؂蔲���E���W
	scissorRect.bottom = scissorRect.top + window_height; //�؂蔲�������W
}