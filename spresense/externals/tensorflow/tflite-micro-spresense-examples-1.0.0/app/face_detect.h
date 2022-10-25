#ifndef TENSORFLOW_LITE_MICRO_FACE_DETECTION
#define TENSORFLOW_LITE_MICRO_FACE_DETECTION

// Expose a C friendly interface for main functions.
#ifdef __cplusplus
extern "C" {
#endif

void FaceSetup(void);

void Run(int type, float* output0, float* output1);

#ifdef __cplusplus
}
#endif

#endif  // TENSORFLOW_LITE_MICRO_FACE_DETECTION
