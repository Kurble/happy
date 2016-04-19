# Happy
Deferred PBR rendering engine for DirectX 11.

Features:
- Deferred rendering
- Deferred Screen Space Direction Occlusion
- PBR with normal, roughness and metallic maps
- Cubemap lighting
- Texture loaders
- OBJ loader

TBA:
- Point lights
- Skinmesh format/fbx converter
- Skinmesh rendering

# Setup
Happy is very easy to setup. All you need to do is create a window and an event loop. The rest is handled by happy!
```C++
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	if (FAILED(InitWindow(hInstance, nCmdShow)))
	{
		return 0;
	}

	happy::RenderingContext context;
	happy::DeferredRenderer renderer(&context);
	context.attach(g_hWnd);
	renderer.resize(context.getWidth(), context.getHeight());

	g_pRenderingContext = &context;
	g_pRenderer = &renderer;
	
	happy::PBREnvironment environment(happy::loadCubemapWICFolder(
		&context,
		"Path\\To\\Cubemap", "jpg"));
	environment.convolute(&context, 32, 16);
	renderer.setEnvironment(environment);

	happy::RenderMesh testModel = happy::loadRenderMeshFromObj(
		&context,
		"Path\\To\\Mesh.obj",
		"Path\\To\\Texture.png",
		"Path\\To\\Normals.png");

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			renderer.clear();

			Mat4 world;
			world.identity();
			renderer.pushRenderMesh(testModel, world);
			
			Vec3 pivot = Vec3(0, 0, 1);
			Vec3 eye = pivot + Vec3(
				cosf(-camRotation[0])*cosf(camRotation[1]),
				sinf(-camRotation[0])*cosf(camRotation[1]),
				sinf(camRotation[1])) * zoom;

			Mat4 view, projection;
			view.identity();
			view.lookat(eye, pivot, Vec3(0, 0, 1));
			projection.identity();

			projection.perspective(55, (float)context.getWidth()/(float)context.getHeight(), .1f, 100.0f);

			renderer.setCamera(view, projection);
			renderer.render();
			context.swap();
		}
	}

	return (int)msg.wParam;
}
```
