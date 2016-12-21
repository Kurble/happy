# Happy
Deferred PBR rendering engine for DirectX 11.

Features:
- Deferred rendering
- Screen Space Ambient Occlusion (SSAO)
- Unreal like Temporal Anti Aliasing (TAA)
- PBR using the specular / gloss workflow
- Image based lighting (IBL)
- Texture loaders
- OBJ loader
- Dynamic point lights
- Simple blendable skinned animations

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
	g_pContext = &context;
	context.attach(g_hWnd);
	
	happy::RenderTarget renderTarget(&context, context.getWidth(), context.getHeight(), true);
	g_pRenderTarget = &renderTarget;
	renderTarget.setOutput(context.getBackBuffer());
	
	happy::DeferredRenderer renderer(&context);
	g_pRenderer = &renderer;
	
	happy::Resources resources("Path\\To\\Resources...", &context);
	
	happy::RenderQueue renderQueue;
	
	happy::PBREnvironment environment = resources.getCubemapFolder("cubemap", "jpg");
	environment.convolute(&context, 32, 16);	

	happy::RenderMesh testModel = resources.getRenderMesh("Mesh.obj", "Textures.texdef");

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
			renderQueue.clear();
			renderQueue.setEnvironment(environment);

			bb::mat4 world;
			world.identity();
			renderQueue.pushRenderMesh(testModel, world, 0);
			
			bb::vec3 pivot = bb::vec3(0, 0, 1);
			bb::vec3 eye = pivot + bb::vec3(
				cosf(-camRotation[0])*cosf(camRotation[1]),
				sinf(-camRotation[0])*cosf(camRotation[1]),
				sinf(camRotation[1])) * zoom;

			bb::mat4 view, projection;
			view.identity();
			view.lookat(eye, pivot, bb::vec3(0, 0, 1));
			projection.identity();

			projection.perspective(55, renderTarget.getWidth()/renderTarget.getHeight(), .1f, 100.0f);

			renderTarget.setView(view);
			renderTarget.setProjection(projection);
			renderer.render(&renderQueue, &renderTarget);
			
			context.swap();
		}
	}

	return (int)msg.wParam;
}
```
