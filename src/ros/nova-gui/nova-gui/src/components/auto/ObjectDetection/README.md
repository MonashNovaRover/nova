# Object Detection (YOLO + ONNX Runtime Web)

This folder implements browser-side YOLO inference using ONNX Runtime Web (ORT).
The pipeline runs on live camera video elements, converts frames to tensors, runs
the ONNX model, and draws detections in a canvas overlay.

## Approach

- Camera components register their `<video>` refs via `YoloProvider`.
- `useYoloDetection`:
  - pulls video frames into an `OffscreenCanvas`
  - normalizes pixels to float32
  - builds a tensor shaped `[N, 3, H, W]`
  - runs ORT inference
  - postprocesses YOLO output into boxes/classes/scores
  - updates state for the overlay

## Stack

- React + Vite
- `onnxruntime-web` (WASM by default, optional WebGPU)
- Canvas / OffscreenCanvas for preprocessing

## Why the `ort/` folder lives here

ORT dynamically loads its `.mjs` loader and `.wasm` binary at runtime. Vite
serves files in `public/` as-is, but **they cannot be imported from source
code**. ORT uses dynamic `import()` for its `.mjs` loader, which must be
resolvable as a module in dev. To satisfy this:

- In **dev**, we serve the ORT loader files from:
  `/src/components/auto/ObjectDetection/ort/`
- In **build**, we serve them from:
  `/public/ort/` (copied to `dist/` as-is)

This is also why we set:

- `ort.env.wasm.wasmPaths` in `useYoloDetection.ts`

References:
- Vite public assets behavior:
  https://vite.dev/guide/assets.html
- ORT `env.wasm.wasmPaths`:
  https://onnxruntime.ai/docs/tutorials/web/env-flags-and-session-options.html

## WebGPU vs WASM

We default to WASM for compatibility. WebGPU is **optional** and only enabled
when `VITE_ENABLE_WEBGPU=true`. Many machines/browsers do not have WebGPU
available or may require enabling developer flags.

## Model input shape and batching

The current model expects a fixed batch size of **1**. If you pass multiple
cameras at once, ORT will throw:

```
Got: 14 Expected: 1
```

To keep things working, the hook runs per-video when the model batch is fixed
to 1. If you want batching, export a YOLO model with a dynamic batch dimension
or a fixed batch equal to your camera count.

## Files

- `useYoloDetection.ts`: preprocessing, inference loop, postprocess
- `YoloProvider.tsx`: registers camera refs, provides detections
- `YoloCameraComponent.tsx`: attaches video + overlay
- `ort/`: ORT loader and WASM binaries for dev-time module loading

## Useful links

- ORT env flags and `wasmPaths`: https://onnxruntime.ai/docs/tutorials/web/env-flags-and-session-options.html
- ORT WebGPU execution provider: https://onnxruntime.ai/docs/tutorials/web/ep-webgpu.html
- Vite public assets behavior: https://vite.dev/guide/assets.html
