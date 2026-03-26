// Tell Windows GPU drivers to prefer the discrete (high-performance) GPU when
// the system has both integrated and discrete graphics (common on AMD APU +
// dGPU and NVIDIA Optimus laptops). Without these exports the OpenGL ICD may
// not load at all, leaving only the GDI Generic GL 1.1 software renderer.
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement              = 1;
__declspec(dllexport) int          AmdPowerXpressRequestHighPerformance = 1;
}
