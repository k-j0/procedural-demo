# Procedural Generation Demo
By Jonathan Kings

More about this project on http://www.jonathan-kings.com/portfolio/?page=portfolio&project=procedural

## Running the app
Build for Windows 32 is in exe/ folder. Note that this is a Debug build, as the teaching library DXFramework.lib used isn't available in Release mode yet.

## Building the code
You can build for Windows 32 in Debug mode (again, because of the library the app can't be built in Release or 64-bit modes).
Please note that Visual Studio sometimes proves very tenacious with HLSL files; especially, it might decide it doesn't want to compile them unless you take the time to re-specify the shader model and shader type; this should be 5.0 for all shaders, and the shader type should be Vertex Shader for files ending in _vs, Pixel Shader for files ending in _fs (fragment), Geometry for _gs, Hull for _hs and Domain for _ds.