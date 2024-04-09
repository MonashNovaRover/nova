# WebGL Hooks
Written by Bailey! Please reach out to me if you need help with this topic. It can be very dense.

I have written a suite of hooks to help you do shader programming in react!

This won't go super in depth as to how to do GL code.

## Basic Usage 

Let's make a simple program with a single shader program, consisting of:
- a vertex shader `example.vert`,
- a fragment shader `example.frag`

you should put your shader code into `.glsl`, `.vert`, or `.frag` files. You can get IDE plugins to make these files 
look pretty :)

`example.vert`
```glsl
#version 300 es

precision mediump float;
in vec4 aPosition;
in vec2 aTexCoord;

out vec2 vTexCoord;

void main() {
    gl_Position = aPosition;
    vTexCoord = aTexCoord; // vec2(0.5) + 0.5*aPosition.xy;
}
```

`example.frag`
```glsl
#version 300 es

precision mediump float;
in vec2 vTexCoord;

uniform sampler2D image;
uniform vec2 offset;

out vec4 fragColor;

void main() {
    vec2 samplePoint = vTexCoord + offset;
    samplePoint = vec2(mod(samplePoint.x, 1.0), mod(samplePoint.y, 1.0));

    // Sample at the projected point
    vec4 texCol = texture(image, samplePoint);
    fragColor = texCol;
}
```

#### Importing source code

Once you have your files, you can import them like any other file in typescript:
```ts
import Vert from "./gl/example.vert";
import Frag from "./gl/example.frag";
```
The name of the variable doesn't matter here. I've chosen `Vert` and `Frag` arbitrarily.
These will be strings containing your shader source code, which we will pass to the `useProgram` hook, which compiles 
our shaders.

### `useGL`

This is the main hook used to setup the rendering context (`WebGL2RenderingContext`) we use to make draw calls. 

Unless you know what you're doing, you probably shouldn't mess with it directly. You can pass it to the other custom 
hooks I've documented below.

Inside your function component, this should be your first WebGL hook:
```tsx
const gl = useGL();
```
Whenever you use this hook, you ***must*** define a `<canvas/>` element for it to use to generate the rendering context, 
and display your shaders on. You need to pass it the `gl.canvasRef` given by `useGL`:

```tsx
return (
  // ...
    <canvas ref={gl.canvasRef}/>
  // ...
);
```

Note: If you want to implement your own hooks, the `gl` object is secretly a ref, so it doesn't need to be passed in 
dependency arrays for effects.

### `useProgram`

Now, you can use a hook to compile your shader into a program!

```tsx
const program = useProgram(gl, Vert, Frag);
```

If you have more than one `useProgram` hook, the programs will be rendered on top of each other, with the last hook 
definition being rendered last.

There is an additional suite of hooks just to set up the variables defined by your program.

### `useAttribute`

If you recall, we had some vertex attributes in `example.vert`:
```glsl
in vec4 aPosition;
in vec2 aTexCoord;
```

You can set your vertex attribute with the `useAttribute` hook, making sure you pass: 
- the program, 
- the name of your attribute, and
- either:
  1. A constant vector array 
  2. A factory function to generate the vector array, along with a dependency array

#### Constant Attributes

For attributes that never vary, you can simply pass in an array of vectors as the third argument:

```tsx
useAttribute(program, "aPosition", [
  [1, 1], [-1, 1], [1, -1], [-1, -1]
]);
```

Note: this is the vertex array that you would use if you simply wanted to draw a quad that fills the `<canvas/>`. This 
is extremely useful, and I use it in basically every program.

#### Variable Attributes

You can also have attributes that vary with your component `props`, values from `useState`, or even values from 
[bifrost](./bifrost.md)!

In this example, a variable attribute is set, which depends on some variable `offset`:

```tsx
useAttribute(program, "aTexCoord", () => [
  [1 + offset, 1], [offset, 1], [1 + offset, 0], [offset, 0]
], [offset]);
```

Whenever anything in the dependencies array changes, the program is scheduled to be re-rendered.

Note: This currently only supports `float`, `vec2`, `vec3`, and `vec4` values, with each type differentiated by the 
length of the inner arrays of the attribute value.

### `useUniform`

If you recall, we had a uniform vec2 in `example.frag`:
```glsl
uniform vec2 offset;
```

The `useUniform` hook has the same syntax as `useAttribute`

### `useSampler`

If you recall, we had a uniform sampler in `example.frag`:
```glsl
uniform sampler2D image;
```

We can use `<img>` and `<video>` elements as sampler values for our programs. Videos will automatically re-render the 
program whenever a new frame is played too!
 
The second argument in the hook is the texture unit (which is an `int` starting from 0). This ***must*** be unique. You 
cannot have two `useSampler` hooks that have the same texture unit.

Here's an example using an image as a sampler:
```tsx
import useImageTexture from " ... ";
import imagePath from "... assets/image.png";

// ...

  const image = useImageTexture(imagePath);
  
  useSampler(program, 0, "image", image);
```

Here's an example using a video as a sampler:
```tsx
import useWebcam from "... ";

// ...

  const videoRef = useRef<HTMLVideoElement | null>(null);
  useWebcam(videoRef);

  useSampler(program, 0, "image", videoRef.current);
  
  return (
    // ...
      <video ref={videoRef} autoPlay={true}/>
    // ...
  )
```
Note: video samplers only work if they are added to the DOM.

## More hooks

### `useResolutionUniform`

Sets a uniform of the form:

```glsl
uniform vec2 resolution;
```
to be the width and height of the canvas render target, in pixels, and is updated whenever the canvas is resized.
This is useful if you need to maintain aspect ratios in your shaders. 

You can use it like this:

```ts
useResolutionUniform(gl, program);
```

### `useProgramEffect`

This is how most of the program hooks (`useAttribute`, `useUniform`, `useSampler`) were defined. 

Whenever an item in the dependencies list changes, this will run your code synchronously before the next re-render of 
the program, and schedules the program to be re-rendered.

```ts
useProgramEffect(program, (context: WebGL2RenderingContext, program: WebGLProgram) => {
  // make webgl calls using the given context and program.
}, [/* dependencies go here */])
```

You can use this to make webgl hooks that relate to the program.

Note: The decision to use the same name for the `useProgram` return value, and the argument in the callback for 
`useCallbackEffect` is intentional. You should ***never*** interact with the `useProgram` return value directly inside 
of the `useProgramEffect` callback, as it could cause infinite loops and unexpected behaviour. By hiding the 
`useProgram` return value inside of the callback, I hope to discourage users from making this mistake.

### `useCanvasSize`

Tries to automatically size your canvas element. This is really dodgy and I don't know how to document it properly.

If used incorrectly, the canvas can rapidly expand to a rediculous size, and freeze up the browser tab. Have fun!

```ts
useCanvasSize(gl);
```

This hook should probably be fixed at some point.

### `useAnimationFrame`

Takes the given callback function, and puts it into a loop formed by recursively calling 
[`requestAnimationFrame`](https://developer.mozilla.org/en-US/docs/Web/API/window/requestAnimationFrame).

This is used to form the default render loop in the `useGL()` hook.

### `useImageTexture`

Gets an image from a path, which you can use as an input to `useSampler`.

```ts
const image = useImageTexture(imagePath);
```

### `useVideoTexture`

Same as `useVideoTexture`, but for video files instead.

```ts
const videoRef = useRef<HTMLVideoElement | null>(null);
useVideoTexture(videoRef, videoPath);
```

Note: The video ref must be used in the DOM by a `<video ref={videoRef}/>` for this to work.

### `useWebcam`

Makes the given video ref display the webcam feed

```ts
const videoRef = useRef<HTMLVideoElement | null>(null);
useWebcam(videoRef);
```

Note: The video ref must be used in the DOM by a `<video ref={videoRef}/>` for this to work.
